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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nputils.h"

#define SU(X) ( ((unsigned long long)(X) << 10) >> shift ), units

#ifdef SUPPORT_ATTRIBUTE_ALIAS
void swapinfo () __attribute__ ((weak, alias ("meminfo")));
#else
# pragma weak swapinfo = meminfo
#endif

/*#define PROC_MEMINFO  "/proc/meminfo"*/
static int meminfo_fd = -1;
#define PROC_STAT     "/proc/stat"
static int stat_fd = -1;
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
#define FILE_TO_BUF(filename, fd) do{                           \
    static int local_n;                                         \
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {    \
        fputs("Error: /proc must be mounted\n", stdout);        \
        fflush(NULL);                                           \
        exit(STATE_UNKNOWN);                                    \
    }                                                           \
    lseek(fd, 0L, SEEK_SET);                                    \
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {        \
        perror(filename);                                       \
        fflush(NULL);                                           \
        exit(STATE_UNKNOWN);                                    \
    }                                                           \
    buf[local_n] = '\0';                                        \
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
unsigned long kb_main_shared;
/* old but still kicking -- the important stuff */
unsigned long kb_main_buffers;
unsigned long kb_main_cached;
unsigned long kb_main_free;
unsigned long kb_main_total;
unsigned long kb_swap_free;
unsigned long kb_swap_total;
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
/* derived values */
unsigned long kb_swap_used;
unsigned long kb_main_used;
/* 2.5.41+ */
unsigned long kb_writeback;
unsigned long kb_slab;
unsigned long nr_reversemaps;
unsigned long kb_committed_as;
unsigned long kb_dirty;
unsigned long kb_inactive;
unsigned long kb_mapped;
unsigned long kb_pagetables;
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
unsigned long vm_nr_dirty;           /* dirty writable pages */
unsigned long vm_nr_writeback;       /* pages under writeback */
unsigned long vm_nr_pagecache;       /* pages in pagecache -- gone in 2.5.66+ kernels */
unsigned long vm_nr_page_table_pages;/* pages used for pagetables */
unsigned long vm_nr_reverse_maps;    /* includes PageDirect */
unsigned long vm_nr_mapped;          /* mapped into pagetables */
unsigned long vm_nr_slab;            /* in slab */
unsigned long vm_pgpgin;             /* kB disk reads  (same as 1st num on /proc/stat page line) */
unsigned long vm_pgpgout;            /* kB disk writes (same as 2nd num on /proc/stat page line) */
unsigned long vm_pswpin;             /* swap reads     (same as 1st num on /proc/stat swap line) */
unsigned long vm_pswpout;            /* swap writes    (same as 2nd num on /proc/stat swap line) */
unsigned long vm_pgalloc;            /* page allocations */
unsigned long vm_pgfree;             /* page freeings */
unsigned long vm_pgactivate;         /* pages moved inactive -> active */
unsigned long vm_pgdeactivate;       /* pages moved active -> inactive */
unsigned long vm_pgfault;           /* total faults (major+minor) */
unsigned long vm_pgmajfault;       /* major faults */
unsigned long vm_pgscan;          /* pages scanned by page reclaim */
unsigned long vm_pgrefill;       /* inspected by refill_inactive_zone */
unsigned long vm_pgsteal;       /* total pages reclaimed */
unsigned long vm_kswapd_steal; /* pages reclaimed by kswapd */
/* next 3 as defined by the 2.5.52 kernel */
unsigned long vm_pageoutrun;  /* times kswapd ran page reclaim */
unsigned long vm_allocstall;  /* times a page allocator ran direct reclaim */
unsigned long vm_pgrotated;   /* pages rotated to the tail of the LRU for immediate reclaim */
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

/* Number of swapins and swapouts (since the last boot):*/
unsigned long kb_swap_pageins;
unsigned long kb_swap_pageouts;

/* Number of pageins and pageouts (since the last boot) */
unsigned long kb_mem_pageins;
unsigned long kb_mem_pageouts;

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
meminfo (int cache_is_free)
{
  char namebuf[16];		/* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL };
  mem_table_struct *found;
  char *head;
  char *tail;
  const char* b;
  int need_vmstat_file = 0;

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
    { "SwapCached",    &kb_swap_cached },
    { "SwapFree",      &kb_swap_free },          /* important */
    { "SwapTotal",     &kb_swap_total },         /* important */
    { "VmallocChunk",  &kb_vmalloc_chunk },
    { "VmallocTotal",  &kb_vmalloc_total },
    { "VmallocUsed",   &kb_vmalloc_used },
    { "Writeback",     &kb_writeback },          /* kB version of vmstat nr_writeback */
  };
  const int mem_table_count = sizeof (mem_table) / sizeof (mem_table_struct);

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

  kb_main_used = kb_main_total - kb_main_free;
  if (cache_is_free)
    {
      kb_main_used -= (kb_main_cached + kb_main_buffers);
      kb_main_free += (kb_main_cached + kb_main_buffers);
    }

  kb_swap_used = kb_swap_total - kb_swap_free;

  /* get additional statistics for memory and swap activity */

  FILE_TO_BUF (PROC_STAT, stat_fd);

  b = strstr(buf, "page ");
  if(b)
    sscanf(b, "page %lu %lu", &kb_mem_pageins, &kb_mem_pageouts);
  else
    need_vmstat_file = 1;

  b = strstr(buf, "swap ");
  if(b)
    sscanf(b, "swap %lu %lu", &kb_swap_pageins, &kb_swap_pageouts);
  else
    need_vmstat_file = 1;

  if (need_vmstat_file)  /* Linux 2.5.40-bk4 and above */
    {
      vminfo();

      kb_mem_pageins  = vm_pgpgin;
      kb_mem_pageouts = vm_pgpgout;

      kb_swap_pageins = vm_pswpin;
      kb_swap_pageouts = vm_pswpout;
    }
}

char *
get_memory_status (int status, float percent_used, int shift,
                   const char *units)
{
  char *msg;
  int ret;

  ret = asprintf (&msg, "%s: %.2f%% (%lu kB) used", state_text (status),
                  percent_used, kb_main_used);

  if (ret < 0)
    die (STATE_UNKNOWN, "Error getting memory status\n");
  
  return msg;
}

char *
get_swap_status (int status, float percent_used, int shift,
                 const char *units)
{
  char *msg;
  int ret;

  ret = asprintf (&msg, "%s: %.2f%% (%lu kB) used", state_text (status),
                  percent_used, kb_swap_used);

  if (ret < 0)
    die (STATE_UNKNOWN, "Error getting swap status\n");

  return msg;
}

char *
get_memory_perfdata (int shift, const char *units)
{
  char *msg;
  int ret;

  ret = asprintf (&msg,
                  "mem_total=%Lu%s, mem_used=%Lu%s, mem_free=%Lu%s, "
                  "mem_shared=%Lu%s, mem_buffers=%Lu%s, mem_cached=%Lu%s, "
                  "mem_pageins=%Lu%s, mem_pageouts=%Lu%s\n",
                  SU (kb_main_total), SU (kb_main_used), SU (kb_main_free),
                  SU (kb_main_shared), SU (kb_main_buffers),
                  SU (kb_main_cached),
                  SU (kb_mem_pageins), SU (kb_mem_pageouts));

  if (ret < 0)
    die (STATE_UNKNOWN, "Error getting memory perfdata\n");

  return msg;
}

char *
get_swap_perfdata (int shift, const char *units)
{
  char *msg;
  int ret;

  ret = asprintf (&msg,
                  "swap_total=%Lu%s, swap_used=%Lu%s, swap_free=%Lu%s, "
                  /* The amount of swap, in kB, used as cache memory */
                  "swap_cached=%Lu%s, "
                  "swap_pageins=%Lu%s, swap_pageouts=%Lu%s\n",
                  SU (kb_swap_total), SU (kb_swap_used), SU (kb_swap_free),
                  SU (kb_swap_cached),
                  SU (kb_swap_pageins), SU (kb_swap_pageouts));

  if (ret < 0)
    die (STATE_UNKNOWN, "Error getting swap perfdata\n");

  return msg;
}
