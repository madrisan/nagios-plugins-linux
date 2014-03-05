/*
 * License: GPLv2
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory and swap usage on linux
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
 * This software is based on the source code of the tool "free" (procps 3.2.8)
 */

#include "config.h"

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
#include "thresholds.h"
#include "xalloc.h"

/*#define PROC_MEMINFO  "/proc/meminfo"*/
static int meminfo_fd = -1;
#define PROC_STAT     "/proc/stat"
#define PROC_VMINFO   "/proc/vmstat"
static int vminfo_fd = -1;

/* As of 2.6.24 /proc/meminfo seems to need 888 on 64-bit,
 * and would need 1258 if the obsolete fields were there.
 */
static char buf[2048];

/* This macro opens filename only if necessary and seeks to 0 so
 * that successive calls to the functions are more efficient.
 * It also reads the current contents of the file into the global buf.
 */
#define FILE_TO_BUF(filename, fd) do{                               \
    static int local_n;                                             \
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {        \
        plugin_error (STATE_UNKNOWN, 0,                             \
                      "Error: /proc must be mounted");              \
    }                                                               \
    lseek(fd, 0L, SEEK_SET);                                        \
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {            \
        plugin_error (STATE_UNKNOWN, 0,                             \
                      "Error reading %s", filename);                \
    }                                                               \
    buf[local_n] = '\0';                                            \
}while(0)

/* example data, following junk, with comments added:
 *
 * MemTotal:        61768 kB    old
 * MemFree:          1436 kB    old
 * MemShared:           0 kB    old (now always zero; not calculated)
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

/* obsolete */
static unsigned long kb_main_shared;
/* old but still kicking -- the important stuff */
static unsigned long kb_main_buffers;
static unsigned long kb_main_cached;
static unsigned long kb_main_free;
static unsigned long kb_main_total;
static unsigned long kb_swap_free;
static unsigned long kb_swap_total;
/* recently introduced */
static unsigned long kb_high_free;
static unsigned long kb_high_total;
static unsigned long kb_low_free;
static unsigned long kb_low_total;
/* 2.4.xx era */
static unsigned long kb_active;
static unsigned long kb_inact_laundry;
static unsigned long kb_inact_dirty;
static unsigned long kb_inact_clean;
static unsigned long kb_inact_target;
static unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */
/* 2.5.41+ */
static unsigned long kb_writeback;
static unsigned long kb_slab;
static unsigned long nr_reversemaps;
static unsigned long kb_committed_as;
static unsigned long kb_dirty;
static unsigned long kb_inactive;
static unsigned long kb_mapped;
static unsigned long kb_pagetables;
// seen on a 2.6.x kernel:
static unsigned long kb_vmalloc_chunk;
static unsigned long kb_vmalloc_total;
static unsigned long kb_vmalloc_used;
// seen on 2.6.24-rc6-git12
static unsigned long kb_anon_pages;
static unsigned long kb_bounce;
static unsigned long kb_commit_limit;
static unsigned long kb_nfs_unstable;
static unsigned long kb_swap_reclaimable;
static unsigned long kb_swap_unreclaimable;

/* read /proc/vminfo only for 2.5.41 and above */

typedef struct vm_table_struct {
  const char *name;     /* VM statistic name */
  unsigned long *slot;       /* slot in return struct */
} vm_table_struct;

static int
compare_vm_table_structs (const void *a, const void *b)
{
  return strcmp (((const vm_table_struct*)a)->name, ((const vm_table_struct*)b)->name);
}

/* see include/linux/page-flags.h and mm/page_alloc.c */
static unsigned long vm_nr_dirty;           /* dirty writable pages */
static unsigned long vm_nr_writeback;       /* pages under writeback */
static unsigned long vm_nr_pagecache;       /* pages in pagecache -- gone in 2.5.66+ kernels */
static unsigned long vm_nr_page_table_pages;/* pages used for pagetables */
static unsigned long vm_nr_reverse_maps;    /* includes PageDirect */
static unsigned long vm_nr_mapped;          /* mapped into pagetables */
static unsigned long vm_nr_slab;            /* in slab */
static unsigned long vm_pgpgin;             /* kB disk reads  (same as 1st num on /proc/stat page line) */
static unsigned long vm_pgpgout;            /* kB disk writes (same as 2nd num on /proc/stat page line) */
static unsigned long vm_pswpin;             /* swap reads     (same as 1st num on /proc/stat swap line) */
static unsigned long vm_pswpout;            /* swap writes    (same as 2nd num on /proc/stat swap line) */
static unsigned long vm_pgalloc;            /* page allocations */
static unsigned long vm_pgfree;             /* page freeings */
static unsigned long vm_pgactivate;         /* pages moved inactive -> active */
static unsigned long vm_pgdeactivate;       /* pages moved active -> inactive */
static unsigned long vm_pgfault;           /* total faults (major+minor) */
static unsigned long vm_pgmajfault;       /* major faults */
static unsigned long vm_pgscan;          /* pages scanned by page reclaim */
static unsigned long vm_pgrefill;       /* inspected by refill_inactive_zone */
static unsigned long vm_pgsteal;       /* total pages reclaimed */
static unsigned long vm_kswapd_steal; /* pages reclaimed by kswapd */
/* next 3 as defined by the 2.5.52 kernel */
static unsigned long vm_pageoutrun;  /* times kswapd ran page reclaim */
static unsigned long vm_allocstall;  /* times a page allocator ran direct reclaim */
static unsigned long vm_pgrotated;   /* pages rotated to the tail of the LRU for immediate reclaim */
/* seen on a 2.6.8-rc1 kernel, apparently replacing old fields */
static unsigned long vm_pgalloc_dma;
static unsigned long vm_pgalloc_high;
static unsigned long vm_pgalloc_normal;
static unsigned long vm_pgrefill_dma;
static unsigned long vm_pgrefill_high;
static unsigned long vm_pgrefill_normal;
static unsigned long vm_pgscan_direct_dma;
static unsigned long vm_pgscan_direct_high;
static unsigned long vm_pgscan_direct_normal;
static unsigned long vm_pgscan_kswapd_dma;
static unsigned long vm_pgscan_kswapd_high;
static unsigned long vm_pgscan_kswapd_normal;
static unsigned long vm_pgsteal_dma;
static unsigned long vm_pgsteal_high;
static unsigned long vm_pgsteal_normal;
// seen on a 2.6.8-rc1 kernel
static unsigned long vm_kswapd_inodesteal;
static unsigned long vm_nr_unstable;
static unsigned long vm_pginodesteal;
static unsigned long vm_slabs_scanned;

void
vminfo (void)
{
  char namebuf[16];             /* big enough to hold any row name */
  vm_table_struct findme = { namebuf, NULL };
  vm_table_struct *found;
  char *head;
  char *tail;

  static const vm_table_struct vm_table[] = {
    { "allocstall",           &vm_allocstall },
    { "kswapd_inodesteal",    &vm_kswapd_inodesteal },
    { "kswapd_steal",         &vm_kswapd_steal },
    { "nr_dirty",             &vm_nr_dirty },            /* page version of meminfo Dirty */
    { "nr_mapped",            &vm_nr_mapped },           /* page version of meminfo Mapped */
    { "nr_page_table_pages" , &vm_nr_page_table_pages }, /* same as meminfo PageTables */
    { "nr_pagecache",         &vm_nr_pagecache },        /* gone in 2.5.66+ kernels */
    { "nr_reverse_maps",      &vm_nr_reverse_maps },     /* page version of meminfo ReverseMaps GONE */
    { "nr_slab",              &vm_nr_slab },             /* page version of meminfo Slab */
    { "nr_unstable",          &vm_nr_unstable },
    { "nr_writeback",         &vm_nr_writeback },        /* page version of meminfo Writeback */
    { "pageoutrun",           &vm_pageoutrun },
    { "pgactivate",           &vm_pgactivate },
    { "pgalloc",              &vm_pgalloc },             /* GONE (now separate dma,high,normal) */
    { "pgalloc_dma",          &vm_pgalloc_dma },
    { "pgalloc_high",         &vm_pgalloc_high },
    { "pgalloc_normal",       &vm_pgalloc_normal },
    { "pgdeactivate",         &vm_pgdeactivate },
    { "pgfault",              &vm_pgfault },
    { "pgfree",               &vm_pgfree },
    { "pginodesteal",         &vm_pginodesteal },
    { "pgmajfault",           &vm_pgmajfault },
    { "pgpgin",               &vm_pgpgin },              /* important */
    { "pgpgout",              &vm_pgpgout },             /* important */
    { "pgrefill",             &vm_pgrefill },            /* GONE (now separate dma,high,normal) */
    { "pgrefill_dma",         &vm_pgrefill_dma },
    { "pgrefill_high",        &vm_pgrefill_high },
    { "pgrefill_normal",      &vm_pgrefill_normal },
    { "pgrotated",            &vm_pgrotated },
    { "pgscan",               &vm_pgscan },              /* GONE (now separate direct,kswapd and dma,high,normal) */
    { "pgscan_direct_dma",    &vm_pgscan_direct_dma },
    { "pgscan_direct_high",   &vm_pgscan_direct_high },
    { "pgscan_direct_normal", &vm_pgscan_direct_normal },
    { "pgscan_kswapd_dma",    &vm_pgscan_kswapd_dma },
    { "pgscan_kswapd_high",   &vm_pgscan_kswapd_high },
    { "pgscan_kswapd_normal", &vm_pgscan_kswapd_normal },
    { "pgsteal",              &vm_pgsteal },             /* GONE (now separate dma,high,normal) */
    { "pgsteal_dma",          &vm_pgsteal_dma },
    { "pgsteal_high",         &vm_pgsteal_high },
    { "pgsteal_normal",       &vm_pgsteal_normal },
    { "pswpin",               &vm_pswpin },              /* important */
    { "pswpout",              &vm_pswpout },             /* important */
    { "slabs_scanned",        &vm_slabs_scanned },
  };
  const int vm_table_count = sizeof (vm_table) / sizeof (vm_table_struct);

#if __SIZEOF_LONG__ == 4
  unsigned long long slotll;
#endif

  vm_pgalloc = 0;
  vm_pgrefill = 0;
  vm_pgscan = 0;
  vm_pgsteal = 0;

  FILE_TO_BUF (PROC_VMINFO, vminfo_fd);

  head = buf;
  for (;;)
    {
      tail = strchr (head, ' ');
      if (!tail) break;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
        {
          head = tail+1;
          goto nextline;
        }
      strcpy (namebuf, head);
      found = bsearch (&findme, vm_table, vm_table_count,
                       sizeof (vm_table_struct), compare_vm_table_structs);
      head = tail + 1;
      if (!found) goto nextline;

#if __SIZEOF_LONG__ == 4
      /* A 32 bit kernel would have already truncated the value, a 64 bit kernel
       * doesn't need to.  Truncate here to let 32 bit programs to continue to get
       * truncated values.  It's that or change the API for a larger data type.
       */
      slotll = strtoull (head, &tail, 10);
      *(found->slot) = (unsigned long) slotll;
#else
      *(found->slot) = strtoul (head, &tail, 10);
#endif

nextline:
      tail = strchr (head, '\n');
      if (!tail) break;
      head = tail + 1;
    }

  if (!vm_pgalloc)
    vm_pgalloc  = vm_pgalloc_dma + vm_pgalloc_high + vm_pgalloc_normal;

  if (!vm_pgrefill)
    vm_pgrefill = vm_pgrefill_dma + vm_pgrefill_high + vm_pgrefill_normal;

  if (!vm_pgscan)
    vm_pgscan   = vm_pgscan_direct_dma + vm_pgscan_direct_high + vm_pgscan_direct_normal
                + vm_pgscan_kswapd_dma + vm_pgscan_kswapd_high + vm_pgscan_kswapd_normal;

  if (!vm_pgsteal)
    vm_pgsteal  = vm_pgsteal_dma + vm_pgsteal_high + vm_pgsteal_normal;
}

typedef struct mem_table_struct {
  const char *name;     /* memory type name */
  unsigned long *slot; /* slot in return struct */
} mem_table_struct;

static int
compare_mem_table_structs (const void *a, const void *b)
{
  return strcmp (((const mem_table_struct*)a)->name,
                 ((const mem_table_struct*)b)->name);
}

void
get_meminfo (bool cache_is_free, struct memory_snapshot **memory)
{
  char namebuf[16];		/* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL };
  mem_table_struct *found;
  char *head;
  char *tail;
  struct memory_snapshot *m = *memory;

  static const mem_table_struct mem_table[] = {
    { "Active",        &kb_active },             /* important */
    { "AnonPages",     &kb_anon_pages },
    { "Bounce",        &kb_bounce },
    { "Buffers",       &kb_main_buffers },       /* important */
    { "Cached",        &kb_main_cached },        /* important */
    { "CommitLimit",   &kb_commit_limit },
    { "Committed_AS",  &kb_committed_as },
    { "Dirty",         &kb_dirty },              /* kB version of vmstat nr_dirty */
    { "HighFree",      &kb_high_free },
    { "HighTotal",     &kb_high_total },
    { "Inact_clean",   &kb_inact_clean },
    { "Inact_dirty",   &kb_inact_dirty },
    { "Inact_laundry", &kb_inact_laundry },
    { "Inact_target",  &kb_inact_target },
    { "Inactive",      &kb_inactive },	        /* important */
    { "LowFree",       &kb_low_free },
    { "LowTotal",      &kb_low_total },
    { "Mapped",        &kb_mapped },             /* kB version of vmstat nr_mapped */
    { "MemFree",       &kb_main_free },	        /* important */
    { "MemShared",     &kb_main_shared },        /* important, but now gone! */
    { "MemTotal",      &kb_main_total },	        /* important */
    { "NFS_Unstable",  &kb_nfs_unstable },
    { "PageTables",    &kb_pagetables },	        /* kB version of vmstat nr_page_table_pages */
    { "ReverseMaps",   &nr_reversemaps },        /* same as vmstat nr_page_table_pages */
    { "SReclaimable",  &kb_swap_reclaimable },   /* "swap reclaimable" (dentry and inode structures) */
    { "SUnreclaim",    &kb_swap_unreclaimable },
    { "Slab",          &kb_slab },               /* kB version of vmstat nr_slab */
    { "VmallocChunk",  &kb_vmalloc_chunk },
    { "VmallocTotal",  &kb_vmalloc_total },
    { "VmallocUsed",   &kb_vmalloc_used },
    { "Writeback",     &kb_writeback },          /* kB version of vmstat nr_writeback */
  };
  const int mem_table_count = sizeof (mem_table) / sizeof (mem_table_struct);

  if (!m)
    m = xmalloc (sizeof (struct memory_snapshot));

  FILE_TO_BUF (PROC_MEMINFO, meminfo_fd);

  kb_inactive = ~0UL;

  head = buf;
  for (;;)
    {
      tail = strchr (head, ':');
      if (!tail)
	break;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
	{
	  head = tail + 1;
	  goto nextline;
	}
      strcpy (namebuf, head);
      found = bsearch (&findme, mem_table, mem_table_count,
		       sizeof (mem_table_struct), compare_mem_table_structs);
      head = tail + 1;
      if (!found)
	goto nextline;
      *(found->slot) = strtoul (head, &tail, 10);

    nextline:
      tail = strchr (head, '\n');
      if (!tail)
	break;
      head = tail + 1;
    }

  if (!kb_low_total)
    {				/* low==main except with large-memory support */
      kb_low_total = kb_main_total;
      kb_low_free = kb_main_free;
    }

  if (kb_inactive == ~0UL)
    {
      kb_inactive = kb_inact_dirty + kb_inact_clean + kb_inact_laundry;
    }

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
get_swapinfo (unsigned long *used, unsigned long *total, unsigned long* free,
	      unsigned long *cached)
{
  char namebuf[16];             /* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL };
  mem_table_struct *found;
  char *head;
  char *tail;

  static const mem_table_struct mem_table[] = {
    { "SwapCached",    &kb_swap_cached },
    { "SwapFree",      &kb_swap_free },          /* important */
    { "SwapTotal",     &kb_swap_total },         /* important */
  };
  const int mem_table_count = sizeof (mem_table) / sizeof (mem_table_struct);

  FILE_TO_BUF (PROC_MEMINFO, meminfo_fd);

  head = buf;
  for (;;)
    {
      tail = strchr (head, ':');
      if (!tail)
        break;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
        {
          head = tail + 1;
          goto nextline;
        }
      strcpy (namebuf, head);
      found = bsearch (&findme, mem_table, mem_table_count,
                       sizeof (mem_table_struct), compare_mem_table_structs);
      head = tail + 1;
      if (!found)
        goto nextline;
      *(found->slot) = strtoul (head, &tail, 10);

    nextline:
      tail = strchr (head, '\n');
      if (!tail)
        break;
      head = tail + 1;
    }

  *cached = kb_swap_cached;
  *total = kb_swap_total;
  *free = kb_swap_free;
  *used = kb_swap_total - kb_swap_free;
}

/* Get additional statistics for memory activity
 * Number of swapins and swapouts (since the last boot)	*/

void
get_mempaginginfo (unsigned long *pgpgin, unsigned long *pgpgout)
{
  FILE *f;
  int need_vmstat_file = 1;

  if (!(f = fopen (PROC_STAT, "r")))
    plugin_error (STATE_UNKNOWN, errno, "Error: /proc must be mounted");

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "page %lu %lu", pgpgin, pgpgout) == 1)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose(f);

  if (need_vmstat_file)  /* Linux 2.5.40-bk4 and above */
    {
      vminfo();

      *pgpgin  = vm_pgpgin;
      *pgpgout = vm_pgpgout;
    }
}

/* get additional statistics for swap activity
 * Number of swapins and swapouts (since the last boot)	*/

void
get_swappaginginfo (unsigned long *pswpin, unsigned long *pswpout)
{
  FILE *f;
  int need_vmstat_file = 1;

  if (!(f = fopen (PROC_STAT, "r")))
    plugin_error (STATE_UNKNOWN, errno, "Error: /proc must be mounted");

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "swap %lu %lu", pswpin, pswpout) == 1)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose(f);

  if (need_vmstat_file)  /* Linux 2.5.40-bk4 and above */
    {
      vminfo();

      *pswpin = vm_pswpin;
      *pswpout = vm_pswpout;
    }
}
