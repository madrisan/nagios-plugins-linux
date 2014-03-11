/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking memory and swap usage on linux
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "messages.h"
#include "meminfo.h"
#include "procparser.h"
#include "thresholds.h"
#include "xalloc.h"

/*#define PROC_MEMINFO  "/proc/meminfo"*/

struct proc_sysmem
{
  int refcount;

  /* old but still kicking -- the important stuff */
  unsigned long kb_main_buffers;
  unsigned long kb_main_cached;
  unsigned long kb_main_free;
  unsigned long kb_main_total;
  unsigned long kb_swap_free;
  unsigned long kb_swap_total;
  /* Shmem in 2.6.32+ */
  unsigned long kb_main_shared;
  /* recently introduced */
  unsigned long kb_high_free;
  unsigned long kb_high_total;
  unsigned long kb_low_free;
  unsigned long kb_low_total;
  /* 2.4.xx era */
  unsigned long kb_active;
  unsigned long kb_inact_laundry;
  unsigned long kb_inact_dirty;
  unsigned long kb_inact_clean;
  unsigned long kb_inact_target;
  unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */
  /* 2.5.41+ */
  unsigned long kb_writeback;
  unsigned long kb_slab;
  unsigned long nr_reversemaps;
  unsigned long kb_committed_as;
  unsigned long kb_dirty;
  unsigned long kb_inactive;
  unsigned long kb_mapped;
  unsigned long kb_pagetables;
  /* seen on a 2.6.x kernel: */
  unsigned long kb_vmalloc_chunk;
  unsigned long kb_vmalloc_total;
  unsigned long kb_vmalloc_used;
  /* seen on 2.6.24-rc6-git12 */
  unsigned long kb_anon_pages;
  unsigned long kb_bounce;
  unsigned long kb_commit_limit;
  unsigned long kb_nfs_unstable;
  unsigned long kb_swap_reclaimable;
  unsigned long kb_swap_unreclaimable;
};

/* Allocates space for a new sysmem object.
 * Returns 0 if all went ok. Errors are returned as negative values. */

int
proc_sysmem_new (struct proc_sysmem **sysmem)
{
  struct proc_sysmem *m;

  m = calloc (1, sizeof (struct proc_sysmem));
  if (!m)
    return -ENOMEM;

  m->refcount = 1;

  *sysmem = m;
  return 0;
}

/* Fill the proc_sysmem structure pointed will the values found in the
 * proc filesystem. */

void proc_sysmem_read (struct proc_sysmem *sysmem)
{
  if (sysmem == NULL)
    return;

  const struct proc_table_struct sysmem_table[] = {
    { "Active",        &sysmem->kb_active },            /* important */
    { "AnonPages",     &sysmem->kb_anon_pages },
    { "Bounce",        &sysmem->kb_bounce },
    { "Buffers",       &sysmem->kb_main_buffers },      /* important */
    { "Cached",        &sysmem->kb_main_cached },       /* important */
    { "CommitLimit",   &sysmem->kb_commit_limit },
    { "Committed_AS",  &sysmem->kb_committed_as },
    { "Dirty",         &sysmem->kb_dirty },             /* kB version of vmstat nr_dirty */
    { "HighFree",      &sysmem->kb_high_free },
    { "HighTotal",     &sysmem->kb_high_total },
    { "Inact_clean",   &sysmem->kb_inact_clean },
    { "Inact_dirty",   &sysmem->kb_inact_dirty },
    { "Inact_laundry", &sysmem->kb_inact_laundry },
    { "Inact_target",  &sysmem->kb_inact_target },
    { "Inactive",      &sysmem->kb_inactive },	        /* important */
    { "LowFree",       &sysmem->kb_low_free },
    { "LowTotal",      &sysmem->kb_low_total },
    { "Mapped",        &sysmem->kb_mapped },            /* kB version of vmstat nr_mapped */
    { "MemFree",       &sysmem->kb_main_free },	        /* important */
    { "MemTotal",      &sysmem->kb_main_total },        /* important */
    { "NFS_Unstable",  &sysmem->kb_nfs_unstable },
    { "PageTables",    &sysmem->kb_pagetables },        /* kB version of vmstat nr_page_table_pages */
    { "ReverseMaps",   &sysmem->nr_reversemaps },       /* same as vmstat nr_page_table_pages */
    { "SReclaimable",  &sysmem->kb_swap_reclaimable },  /* "swap reclaimable" (dentry and inode structures) */
    { "SUnreclaim",    &sysmem->kb_swap_unreclaimable },
    { "Shmem",         &sysmem->kb_main_shared},        /* kernel 2.6.32 and later */
    { "Slab",          &sysmem->kb_slab },              /* kB version of vmstat nr_slab */
    { "SwapCached",    &sysmem->kb_swap_cached },       /* late 2.4 and 2.6+ only */
    { "SwapFree",      &sysmem->kb_swap_free },         /* important */
    { "SwapTotal",     &sysmem->kb_swap_total },        /* important */
    { "VmallocChunk",  &sysmem->kb_vmalloc_chunk },
    { "VmallocTotal",  &sysmem->kb_vmalloc_total },
    { "VmallocUsed",   &sysmem->kb_vmalloc_used },
    { "Writeback",     &sysmem->kb_writeback },         /* kB version of vmstat nr_writeback */
  };
  const int sysmem_table_count = sizeof (sysmem_table) / sizeof (proc_table_struct);

  sysmem->kb_inactive = ~0UL;
  procparser (PROC_MEMINFO, sysmem_table, sysmem_table_count, ':');

  if (!sysmem->kb_low_total)
    {				/* low==main except with large-memory support */
      sysmem->kb_low_total = sysmem->kb_main_total;
      sysmem->kb_low_free = sysmem->kb_main_free;
    }

  if (sysmem->kb_inactive == ~0UL)
    {
      sysmem->kb_inactive =
	sysmem->kb_inact_dirty + sysmem->kb_inact_clean +
	sysmem->kb_inact_laundry;
    }

  /* "Cached" includes "Shmem" - we want only the page cache here */
  sysmem->kb_main_cached -= sysmem->kb_main_shared;
}

/* Drop a reference of the memory library context. If the refcount of
 * reaches zero, the resources of the context will be released.  */
struct proc_sysmem *proc_sysmem_unref (struct proc_sysmem *sysmem)
{
  if (sysmem == NULL)
    return NULL;

  sysmem->refcount--;
  if (sysmem->refcount > 0)
    return sysmem;

  free (sysmem);
  return NULL;
}

#define proc_sysmem_get(arg)                    \
unsigned long                                   \
proc_sysmem_get_ ## arg (struct proc_sysmem *p) \
{                                               \
  return (p == NULL) ? 0 : p->kb_ ## arg;       \
}

proc_sysmem_get(main_buffers)
proc_sysmem_get(main_cached)
proc_sysmem_get(main_free)
proc_sysmem_get(main_shared)
proc_sysmem_get(main_total)
proc_sysmem_get(swap_cached)
proc_sysmem_get(swap_free)
proc_sysmem_get(swap_total)

unsigned long
proc_sysmem_get_main_used (struct proc_sysmem *sysmem)
{
  return (sysmem == NULL) ? 0 : sysmem->kb_main_total - sysmem->kb_main_free;
}

unsigned long
proc_sysmem_get_swap_used (struct proc_sysmem *sysmem)
{
  return (sysmem == NULL) ? 0 : sysmem->kb_swap_total - sysmem->kb_swap_free;
}
