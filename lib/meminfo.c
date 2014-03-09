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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This module is based on the code of procps 3.2.8
 */

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
#define PROC_STAT     "/proc/stat"
#define PROC_VMSTAT   "/proc/vmstat"

struct proc_vminfo
{
  /* read /proc/vminfo only for 2.5.41 and above */
  /* see include/linux/page-flags.h and mm/page_alloc.c */
  unsigned long nr_dirty;           /* dirty writable pages */
  unsigned long nr_writeback;       /* pages under writeback */
  unsigned long nr_pagecache;       /* pages in pagecache -- gone in 2.5.66+ kernels */
  unsigned long nr_page_table_pages;/* pages used for pagetables */
  unsigned long nr_reverse_maps;    /* includes PageDirect */
  unsigned long nr_mapped;          /* mapped into pagetables */
  unsigned long nr_slab;            /* in slab */
  unsigned long pgpgin;             /* kB disk reads  (same as 1st num on /proc/stat page line) */
  unsigned long pgpgout;            /* kB disk writes (same as 2nd num on /proc/stat page line) */
  unsigned long pswpin;             /* swap reads     (same as 1st num on /proc/stat swap line) */
  unsigned long pswpout;            /* swap writes    (same as 2nd num on /proc/stat swap line) */
  unsigned long pgalloc;            /* page allocations */
  unsigned long pgfree;             /* page freeings */
  unsigned long pgactivate;         /* pages moved inactive -> active */
  unsigned long pgdeactivate;       /* pages moved active -> inactive */
  unsigned long pgfault;           /* total faults (major+minor) */
  unsigned long pgmajfault;       /* major faults */
  unsigned long pgscan;          /* pages scanned by page reclaim */
  unsigned long pgrefill;       /* inspected by refill_inactive_zone */
  unsigned long pgsteal;       /* total pages reclaimed */
  unsigned long kswapd_steal; /* pages reclaimed by kswapd */
  /* next 3 as defined by the 2.5.52 kernel */
  unsigned long pageoutrun;  /* times kswapd ran page reclaim */
  unsigned long allocstall;  /* times a page allocator ran direct reclaim */
  unsigned long pgrotated;   /* pages rotated to the tail of the LRU for immediate reclaim */
  /* seen on a 2.6.8-rc1 kernel, apparently replacing old fields */
  unsigned long pgalloc_dma;
  unsigned long pgalloc_high;
  unsigned long pgalloc_normal;
  unsigned long pgrefill_dma;
  unsigned long pgrefill_high;
  unsigned long pgrefill_normal;
  unsigned long pgscan_direct_dma;
  unsigned long pgscan_direct_high;
  unsigned long pgscan_direct_normal;
  unsigned long pgscan_kswapd_dma;
  unsigned long pgscan_kswapd_high;
  unsigned long pgscan_kswapd_normal;
  unsigned long pgsteal_dma;
  unsigned long pgsteal_high;
  unsigned long pgsteal_normal;
  /* seen on a 2.6.8-rc1 kernel */
  unsigned long kswapd_inodesteal;
  unsigned long nr_unstable;
  unsigned long pginodesteal;
  unsigned long slabs_scanned;
};

void
get_vminfo (struct proc_vminfo *vm)
{
  const proc_table_struct vm_table[] = {
    { "allocstall",           &vm->allocstall },
    { "kswapd_inodesteal",    &vm->kswapd_inodesteal },
    { "kswapd_steal",         &vm->kswapd_steal },
    { "nr_dirty",             &vm->nr_dirty },        /* page version of meminfo Dirty */
    { "nr_mapped",            &vm->nr_mapped },       /* page version of meminfo Mapped */
    { "nr_page_table_pages" , &vm->nr_page_table_pages }, /* same as meminfo PageTables */
    { "nr_pagecache",         &vm->nr_pagecache },        /* gone in 2.5.66+ kernels */
    { "nr_reverse_maps",      &vm->nr_reverse_maps }, /* page version of meminfo ReverseMaps GONE */
    { "nr_slab",              &vm->nr_slab },         /* page version of meminfo Slab */
    { "nr_unstable",          &vm->nr_unstable },
    { "nr_writeback",         &vm->nr_writeback },    /* page version of meminfo Writeback */
    { "pageoutrun",           &vm->pageoutrun },
    { "pgactivate",           &vm->pgactivate },
    { "pgalloc",              &vm->pgalloc }, /* GONE (now separate dma,high,normal) */
    { "pgalloc_dma",          &vm->pgalloc_dma },
    { "pgalloc_high",         &vm->pgalloc_high },
    { "pgalloc_normal",       &vm->pgalloc_normal },
    { "pgdeactivate",         &vm->pgdeactivate },
    { "pgfault",              &vm->pgfault },
    { "pgfree",               &vm->pgfree },
    { "pginodesteal",         &vm->pginodesteal },
    { "pgmajfault",           &vm->pgmajfault },
    { "pgpgin",               &vm->pgpgin },              /* important */
    { "pgpgout",              &vm->pgpgout },             /* important */
    { "pgrefill",             &vm->pgrefill }, /* GONE (now separate dma,high,normal) */
    { "pgrefill_dma",         &vm->pgrefill_dma },
    { "pgrefill_high",        &vm->pgrefill_high },
    { "pgrefill_normal",      &vm->pgrefill_normal },
    { "pgrotated",            &vm->pgrotated },
    { "pgscan",               &vm->pgscan }, /* GONE (now separate direct,kswapd and dma,high,normal) */
    { "pgscan_direct_dma",    &vm->pgscan_direct_dma },
    { "pgscan_direct_high",   &vm->pgscan_direct_high },
    { "pgscan_direct_normal", &vm->pgscan_direct_normal },
    { "pgscan_kswapd_dma",    &vm->pgscan_kswapd_dma },
    { "pgscan_kswapd_high",   &vm->pgscan_kswapd_high },
    { "pgscan_kswapd_normal", &vm->pgscan_kswapd_normal },
    { "pgsteal",              &vm->pgsteal }, /* GONE (now separate dma,high,normal) */
    { "pgsteal_dma",          &vm->pgsteal_dma },
    { "pgsteal_high",         &vm->pgsteal_high },
    { "pgsteal_normal",       &vm->pgsteal_normal },
    { "pswpin",               &vm->pswpin },              /* important */
    { "pswpout",              &vm->pswpout },             /* important */
    { "slabs_scanned",        &vm->slabs_scanned },
  };
  const int vm_table_count =
    sizeof (vm_table) / sizeof (proc_table_struct);

  vm->pgalloc = 0;
  vm->pgrefill = 0;
  vm->pgscan = 0;
  vm->pgsteal = 0;

  procparser (PROC_VMSTAT, vm_table, vm_table_count, ' ');

  if (!vm->pgalloc)
    vm->pgalloc = vm->pgalloc_dma + vm->pgalloc_high + vm->pgalloc_normal;

  if (!vm->pgrefill)
    vm->pgrefill = vm->pgrefill_dma + vm->pgrefill_high + vm->pgrefill_normal;

  if (!vm->pgscan)
    vm->pgscan = vm->pgscan_direct_dma + vm->pgscan_direct_high
		+ vm->pgscan_direct_normal + vm->pgscan_kswapd_dma
		+ vm->pgscan_kswapd_high + vm->pgscan_kswapd_normal;

  if (!vm->pgsteal)
    vm->pgsteal  = vm->pgsteal_dma + vm->pgsteal_high + vm->pgsteal_normal;
}

void
get_meminfo (bool cache_is_free, struct memory_status **memory)
{
  struct memory_status *m = *memory;

  /* example data, following junk, with comments added:
   *
   * MemTotal:        61768 kB    old
   * MemFree:          1436 kB    old
   * Buffers:          1312 kB    old
   * Cached:          20932 kB    old
   * Active:          12464 kB    new
   * Inact_dirty:      7772 kB    new
   * Inact_clean:      2008 kB    new
   * Inact_target:        0 kB    new
   * Inact_laundry:       0 kB    new, and might be missing too
   * HighTotal:           0 kB
   * HighFree:            0 kB
   * LowTotal:        61768 kB
   * LowFree:          1436 kB
   * SwapTotal:      122580 kB    old
   * SwapFree:        60352 kB    old
   * Inactive:        20420 kB    2.5.41+
   * Dirty:               0 kB    2.5.41+
   * Writeback:           0 kB    2.5.41+
   * Mapped:           9792 kB    2.5.41+
   * Slab:             4564 kB    2.5.41+
   * Committed_AS:     8440 kB    2.5.41+
   * PageTables:        304 kB    2.5.41+
   * ReverseMaps:      5738       2.5.41+
   * SwapCached:          0 kB    2.5.??+
   * HugePages_Total:   220       2.5.??+
   * HugePages_Free:    138       2.5.??+
   * Hugepagesize:     4096 kB    2.5.??+
   */

  /* Shmem in 2.6.32+ */
  unsigned long kb_main_shared = 0;
  /* old but still kicking -- the important stuff */
  unsigned long kb_main_buffers;
  unsigned long kb_main_cached;
  unsigned long kb_main_free;
  unsigned long kb_main_total;
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

  const proc_table_struct meminfo_table[] = {
    { "Active",        &kb_active },            /* important */
    { "AnonPages",     &kb_anon_pages },
    { "Bounce",        &kb_bounce },
    { "Buffers",       &kb_main_buffers },      /* important */
    { "Cached",        &kb_main_cached },       /* important */
    { "CommitLimit",   &kb_commit_limit },
    { "Committed_AS",  &kb_committed_as },
    { "Dirty",         &kb_dirty },             /* kB version of vmstat nr_dirty */
    { "HighFree",      &kb_high_free },
    { "HighTotal",     &kb_high_total },
    { "Inact_clean",   &kb_inact_clean },
    { "Inact_dirty",   &kb_inact_dirty },
    { "Inact_laundry", &kb_inact_laundry },
    { "Inact_target",  &kb_inact_target },
    { "Inactive",      &kb_inactive },	        /* important */
    { "LowFree",       &kb_low_free },
    { "LowTotal",      &kb_low_total },
    { "Mapped",        &kb_mapped },            /* kB version of vmstat nr_mapped */
    { "MemFree",       &kb_main_free },	        /* important */
    { "MemTotal",      &kb_main_total },        /* important */
    { "NFS_Unstable",  &kb_nfs_unstable },
    { "PageTables",    &kb_pagetables },        /* kB version of vmstat nr_page_table_pages */
    { "ReverseMaps",   &nr_reversemaps },       /* same as vmstat nr_page_table_pages */
    { "SReclaimable",  &kb_swap_reclaimable },  /* "swap reclaimable" (dentry and inode structures) */
    { "SUnreclaim",    &kb_swap_unreclaimable },
    { "Shmem",         &kb_main_shared},        /* kernel 2.6.32 and later */
    { "Slab",          &kb_slab },              /* kB version of vmstat nr_slab */
    { "VmallocChunk",  &kb_vmalloc_chunk },
    { "VmallocTotal",  &kb_vmalloc_total },
    { "VmallocUsed",   &kb_vmalloc_used },
    { "Writeback",     &kb_writeback },         /* kB version of vmstat nr_writeback */
  };
  const int meminfo_table_count =
    sizeof (meminfo_table) / sizeof (proc_table_struct);

  if (!m)
    m = xmalloc (sizeof (struct memory_status));

  kb_inactive = ~0UL;

  procparser (PROC_MEMINFO, meminfo_table, meminfo_table_count, ':');

  if (!kb_low_total)
    {				/* low==main except with large-memory support */
      kb_low_total = kb_main_total;
      kb_low_free = kb_main_free;
    }

  if (kb_inactive == ~0UL)
    {
      kb_inactive = kb_inact_dirty + kb_inact_clean + kb_inact_laundry;
    }

  /* "Cached" includes "Shmem" - we want only the page cache here */
  kb_main_cached -= kb_main_shared;

  m->used = kb_main_total - kb_main_free;
  m->total = kb_main_total;
  m->free = kb_main_free;
  m->shared = kb_main_shared;
  m->buffers = kb_main_buffers;
  m->cached = kb_main_cached;

  if (cache_is_free)
    {
      (*memory)->used -= (kb_main_cached + kb_main_buffers);
      (*memory)->free += (kb_main_cached + kb_main_buffers);
    }

  *memory = m;
}

void
get_swapinfo (struct swap_status **swap)
{
  struct swap_status *s = *swap;

  /* old but still kicking -- the important stuff */
  unsigned long kb_swap_free;
  unsigned long kb_swap_total;
  /* 2.4.xx era */
  unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */

  const proc_table_struct proc_table[] = {
    { "SwapCached",    &kb_swap_cached },        /* late 2.4 and 2.6+ only */
    { "SwapFree",      &kb_swap_free },          /* important */
    { "SwapTotal",     &kb_swap_total },         /* important */
  };
  const int proc_table_count =
    sizeof (proc_table) / sizeof (proc_table_struct);

  if (!s)
    s = xmalloc (sizeof (struct swap_status));

  procparser (PROC_MEMINFO, proc_table, proc_table_count, ':');

  s->cached = kb_swap_cached;
  s->total = kb_swap_total;
  s->free = kb_swap_free;
  s->used = kb_swap_total - kb_swap_free;

  *swap = s;
}

static char buf[2048];

/* Get additional statistics for memory activity
 * Number of swapins and swapouts (since the last boot)	*/

void
get_mempaginginfo (unsigned long *pgpgin, unsigned long *pgpgout)
{
  FILE *f;
  int need_vmstat_file = 1;
  struct proc_vminfo *vm;

  if (!(f = fopen (PROC_STAT, "r")))
    plugin_error (STATE_UNKNOWN, errno, "Error: /proc must be mounted");

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "page %lu %lu", pgpgin, pgpgout) == 2)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose(f);

  if (need_vmstat_file)  /* Linux 2.5.40-bk4 and above */
    {
      vm = xmalloc (sizeof (struct proc_vminfo));
      get_vminfo(vm);

      *pgpgin  = vm->pgpgin;
      *pgpgout = vm->pgpgout;

      free (vm);
    }
}

/* get additional statistics for swap activity
 * Number of swapins and swapouts (since the last boot)	*/

void
get_swappaginginfo (unsigned long *pswpin, unsigned long *pswpout)
{
  FILE *f;
  int need_vmstat_file = 1;
  struct proc_vminfo *vm;

  if (!(f = fopen (PROC_STAT, "r")))
    plugin_error (STATE_UNKNOWN, errno, "Error: /proc must be mounted");

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "swap %lu %lu", pswpin, pswpout) == 2)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose(f);

  if (need_vmstat_file)  /* Linux 2.5.40-bk4 and above */
    {
      vm = xmalloc (sizeof (struct proc_vminfo));
      get_vminfo(vm);

      *pswpin = vm->pswpin;
      *pswpout = vm->pswpout;

      free (vm);
    }
}
