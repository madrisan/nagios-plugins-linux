// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014-2015 Davide Madrisan <davide.madrisan@gmail.com>
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <linux/version.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "getenv.h"
#include "kernelver.h"
#include "logging.h"
#include "messages.h"
#include "meminfo.h"
#include "procparser.h"
#include "sysfsparser.h"
#include "system.h"
#include "units.h"

#define MEMINFO_UNSET ~0UL

#define PATH_PROC_SYS		"/proc/sys"
#define PATH_VM_MIN_FREE_KB	PATH_PROC_SYS "/vm/min_free_kbytes"

typedef struct proc_sysmem_data
{
  /* old but still kicking -- the important stuff */
  unsigned long kb_main_buffers;
  unsigned long kb_page_cache;
  unsigned long kb_main_free;
  unsigned long kb_main_total;
  unsigned long kb_swap_free;
  unsigned long kb_swap_total;
  /* Shmem in 2.6.32+ */
  unsigned long kb_main_shared;
  /* recently introduced */
  unsigned long kb_high_total;
  unsigned long kb_low_free;
  unsigned long kb_low_total;
  /* 2.4.xx era */
  unsigned long kb_active;
  unsigned long kb_inact_laundry;
  unsigned long kb_inact_dirty;
  unsigned long kb_inact_clean;
  unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */
  /* 2.5.41+ */
  unsigned long kb_slab;
  unsigned long kb_committed_as;
  unsigned long kb_dirty;
  unsigned long kb_inactive;
  // 2.6.19+
  unsigned long kb_slab_reclaimable;
  /* seen on 2.6.24-rc6-git12 */
  unsigned long kb_anon_pages;
  // 2.6.27+
  unsigned long kb_active_file;
  unsigned long kb_inactive_file;
  /* 3.14+
   * MemAvailable provides an estimate of how much memory is available for
   * starting new applications, without swapping.
   * However, unlike the data provided by the Cache or Free fields,
   * MemAvailable takes into account page cache and also that not all
   * reclaimable memory slabs will be reclaimable due to items being in
   * use.
   * See: https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/\
   *  commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
   */
  unsigned long kb_main_available;
  /* derived values */
  unsigned long kb_main_cached;
  unsigned long kb_main_used;
} proc_sysmem_data_t;

typedef struct proc_sysmem
{
  int refcount;
  struct proc_sysmem_data *data;
} proc_sysmem_t;

#ifndef NPL_TESTING

const char *
get_path_proc_meminfo ()
{
  const char *env_procmeminfo = secure_getenv ("NPL_TEST_PATH_PROCMEMINFO");
  if (env_procmeminfo)
    return env_procmeminfo;

  return PROC_MEMINFO;
}

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
  m->data = calloc (1, sizeof (struct proc_sysmem_data));
  if (!m->data)
    {
      free (m);
      return -ENOMEM;
    }

  *sysmem = m;
  return 0;
}

/* Fill the proc_sysmem structure pointed will the values found in the
 * proc filesystem. */

void proc_sysmem_read (struct proc_sysmem *sysmem)
{
  if (sysmem == NULL)
    return;

  struct proc_sysmem_data *data = sysmem->data;

  const struct proc_table_struct sysmem_table[] = {
    { "Active", &data->kb_active },        /* important */
    { "Active(file)", &data->kb_active_file },
    { "AnonPages", &data->kb_anon_pages },
    { "Buffers", &data->kb_main_buffers }, /* important */
    { "Cached", &data->kb_page_cache },   /* important */
    { "Committed_AS", &data->kb_committed_as },
    { "Dirty", &data->kb_dirty },  /* kB version of vmstat nr_dirty */
    { "HighTotal", &data->kb_high_total },
    { "Inact_clean", &data->kb_inact_clean },
    { "Inact_dirty", &data->kb_inact_dirty },
    { "Inact_laundry", &data->kb_inact_laundry },
    { "Inactive", &data->kb_inactive },	 /* important */
    { "Inactive(file)", &data->kb_inactive_file },
    { "LowFree", &data->kb_low_free },
    { "LowTotal", &data->kb_low_total },
    { "MemAvailable", &data->kb_main_available },  /* kernel 3.14 and later */
    { "MemFree", &data->kb_main_free },	    /* important */
    { "MemTotal", &data->kb_main_total },    /* important */
    { "SReclaimable", &data->kb_slab_reclaimable }, /* dentry and inode structures */
    { "Shmem", &data->kb_main_shared },       /* kernel 2.6.32 and later */
    { "Slab", &data->kb_slab },               /* kB version of vmstat nr_slab */
    { "SwapCached", &data->kb_swap_cached },  /* late 2.4 and 2.6+ only */
    { "SwapFree", &data->kb_swap_free },      /* important */
    { "SwapTotal", &data->kb_swap_total },    /* important */
  };
  const int sysmem_table_count = sizeof (sysmem_table) / sizeof (proc_table_struct);

  data->kb_inactive = MEMINFO_UNSET;
  data->kb_low_total = MEMINFO_UNSET;
  data->kb_main_available = MEMINFO_UNSET;

  procparser (get_path_proc_meminfo (), sysmem_table, sysmem_table_count, ':');

  if (!data->kb_low_total)
    {			       /* low==main except with large-memory support */
      data->kb_low_total = data->kb_main_total;
      data->kb_low_free = data->kb_main_free;
    }

  if (data->kb_inactive == MEMINFO_UNSET)
    {
      data->kb_inactive =
	data->kb_inact_dirty + data->kb_inact_clean +
	data->kb_inact_laundry;
    }

  /* zero? might need fallback for 2.6.27 <= kernel < 3.14 */
  if (data->kb_main_available == MEMINFO_UNSET)
    {
      dbg ("MemAvailable is not provided by /proc/meminfo (kernel %u)...",
	   linux_version ());
      if (linux_version () < KERNEL_VERSION (2, 6, 27))
	{
	  dbg ("...falling back to MemFree\n");
	  data->kb_main_available = data->kb_main_free;
	}
      else
	{
	  dbg ("...let's calculate the value...\n");
	  unsigned long long kb_min_free;
	  sysfsparser_getvalue (&kb_min_free, PATH_VM_MIN_FREE_KB);
	  /* should be equal to sum of all 'low' fields in /proc/zoneinfo */
	  unsigned long watermark_low = kb_min_free * 5 / 4;

	  signed long mem_available =
	    (signed long) data->kb_main_free - watermark_low
	    + data->kb_inactive_file + data->kb_active_file
#define MIN(x,y) ((x) < (y) ? (x) : (y))
	    - MIN((data->kb_inactive_file + data->kb_active_file) / 2,
		   watermark_low) + data->kb_slab_reclaimable
	    - MIN(data->kb_slab_reclaimable / 2, watermark_low);
#undef MIN
	  if (mem_available < 0)
	    mem_available = 0;
	  data->kb_main_available = (unsigned long) mem_available;
	}
    }

  /* derived values */
  data->kb_main_cached = data->kb_page_cache + data->kb_slab_reclaimable;
  data->kb_main_used =
    data->kb_main_total - data->kb_main_free - data->kb_main_cached -
    data->kb_main_buffers;
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

  free (sysmem->data);
  free (sysmem);
  return NULL;
}

#define proc_sysmem_get(arg) \
unsigned long proc_sysmem_get_ ## arg (struct proc_sysmem *p) \
  { return (p == NULL) ? 0 : p->data->kb_ ## arg; }

/* Memory that has been used more recently and usually not reclaimed unless
 * absolutely necessary.  */
proc_sysmem_get(active)
/* Non-file backed pages mapped into user-space page tables.
 * High AnonPages means too much memory been allocated (mostly by malloc call)
 * but not released yet (by instance because of memory leaks - process(es)
 * that allocate but never free memory.  */
proc_sysmem_get(anon_pages)
/* The amount of memory presently allocated on the system. */
proc_sysmem_get(committed_as)
/* Memory which is waiting to get written back to the disk. */
proc_sysmem_get(dirty)
/* Memory which has been less recently used.
 * Inactive memory is the best candidate for reclaiming memory and so low
 * inactive memory would mean that you are low on memory and the kernel may
 + have to swap out process pages, or swap out the cache to disk or in the
 * worst case if it runs out of swap space then begin killing processes.  */
proc_sysmem_get(inactive)
proc_sysmem_get(main_available)
proc_sysmem_get(main_buffers)
proc_sysmem_get(main_cached)
proc_sysmem_get(main_free)
proc_sysmem_get(main_shared)
proc_sysmem_get(main_total)
proc_sysmem_get(main_used)

proc_sysmem_get(swap_cached)
proc_sysmem_get(swap_free)
proc_sysmem_get(swap_total)

#undef proc_sysmem_get

unsigned long
proc_sysmem_get_swap_used (struct proc_sysmem *sysmem)
{
  return (sysmem == NULL) ? 0 :
    sysmem->data->kb_swap_total - sysmem->data->kb_swap_free;
}

#endif			/* NPL_TESTING */
