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
 * This module is based on procps 3.2.8
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "messages.h"
#include "meminfo.h"
#include "procparser.h"

#define PROC_STAT     "/proc/stat"
#define PROC_VMSTAT   "/proc/vmstat"

struct proc_vmem
{
  int refcount;

  /* read /proc/vminfo only for 2.5.41 and above */
  /* see include/linux/page-flags.h and mm/page_alloc.c */
  unsigned long vm_nr_dirty;	/* dirty writable pages */
  unsigned long vm_nr_writeback;	/* pages under writeback */
  unsigned long vm_nr_pagecache;	/* pages in pagecache -- gone in 2.5.66+ kernels */
  unsigned long vm_nr_page_table_pages;	/* pages used for pagetables */
  unsigned long vm_nr_reverse_maps;	/* includes PageDirect */
  unsigned long vm_nr_mapped;	/* mapped into pagetables */
  unsigned long vm_nr_slab;	/* in slab */
  unsigned long vm_pgpgin;	/* kB disk reads  (same as 1st num on /proc/stat page line) */
  unsigned long vm_pgpgout;	/* kB disk writes (same as 2nd num on /proc/stat page line) */
  unsigned long vm_pswpin;	/* swap reads     (same as 1st num on /proc/stat swap line) */
  unsigned long vm_pswpout;	/* swap writes    (same as 2nd num on /proc/stat swap line) */
  unsigned long vm_pgalloc;	/* page allocations */
  unsigned long vm_pgfree;	/* page freeings */
  unsigned long vm_pgactivate;	/* pages moved inactive -> active */
  unsigned long vm_pgdeactivate;	/* pages moved active -> inactive */
  unsigned long vm_pgfault;	/* total faults (major+minor) */
  unsigned long vm_pgmajfault;	/* major faults */
  unsigned long vm_pgscan;	/* pages scanned by page reclaim */
  unsigned long vm_pgrefill;	/* inspected by refill_inactive_zone */
  unsigned long vm_pgsteal;	/* total pages reclaimed */
  unsigned long vm_kswapd_steal;	/* pages reclaimed by kswapd */
  /* next 3 as defined by the 2.5.52 kernel */
  unsigned long vm_pageoutrun;	/* times kswapd ran page reclaim */
  unsigned long vm_allocstall;	/* times a page allocator ran direct reclaim */
  unsigned long vm_pgrotated;	/* pages rotated to the tail of the LRU for immediate reclaim */
  /* seen on a 2.6.8-rc1 kernel, apparently replacing old fields */
  unsigned long vm_pgalloc_dma;
  unsigned long vm_pgalloc_high;
  unsigned long vm_pgalloc_normal;
  unsigned long vm_pgrefill_dma;
  unsigned long vm_pgrefill_high;
  unsigned long vm_pgrefill_normal;
  unsigned long vm_pgscan_direct_dma;
  unsigned long vm_pgscan_direct_high;
  unsigned long vm_pgscan_direct_normal;
  unsigned long vm_pgscan_kswapd_dma;
  unsigned long vm_pgscan_kswapd_high;
  unsigned long vm_pgscan_kswapd_normal;
  unsigned long vm_pgsteal_dma;
  unsigned long vm_pgsteal_high;
  unsigned long vm_pgsteal_normal;
  /* seen on a 2.6.8-rc1 kernel */
  unsigned long vm_kswapd_inodesteal;
  unsigned long vm_nr_unstable;
  unsigned long vm_pginodesteal;
  unsigned long vm_slabs_scanned;
};

/* Allocates space for a new vmem object.
 * Returns 0 if all went ok. Errors are returned as negative values. */

int
proc_vmem_new (struct proc_vmem **vmem)
{
  struct proc_vmem *vm;

  vm = calloc (1, sizeof (struct proc_vmem));
  if (!vm)
    return -ENOMEM;

  vm->refcount = 1;

  *vmem = vm;
  return 0;
}

/* Fill the proc_vmem structure pointed will the values found in the
 * proc filesystem. */

void
proc_vmem_read (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return;

  const proc_table_struct vmem_table[] = {
    {"allocstall", &vmem->vm_allocstall},
    {"kswapd_inodesteal", &vmem->vm_kswapd_inodesteal},
    {"kswapd_steal", &vmem->vm_kswapd_steal},
    {"nr_dirty", &vmem->vm_nr_dirty},	/* page version of meminfo Dirty */
    {"nr_mapped", &vmem->vm_nr_mapped},	/* page version of meminfo Mapped */
    {"nr_page_table_pages", &vmem->vm_nr_page_table_pages},	/* same as meminfo PageTables */
    {"nr_pagecache", &vmem->vm_nr_pagecache},	/* gone in 2.5.66+ kernels */
    {"nr_reverse_maps", &vmem->vm_nr_reverse_maps},	/* page version of meminfo ReverseMaps GONE */
    {"nr_slab", &vmem->vm_nr_slab},	/* page version of meminfo Slab */
    {"nr_unstable", &vmem->vm_nr_unstable},
    {"nr_writeback", &vmem->vm_nr_writeback},	/* page version of meminfo Writeback */
    {"pageoutrun", &vmem->vm_pageoutrun},
    {"pgactivate", &vmem->vm_pgactivate},
    {"pgalloc", &vmem->vm_pgalloc},	/* GONE (now separate dma,high,normal) */
    {"pgalloc_dma", &vmem->vm_pgalloc_dma},
    {"pgalloc_high", &vmem->vm_pgalloc_high},
    {"pgalloc_normal", &vmem->vm_pgalloc_normal},
    {"pgdeactivate", &vmem->vm_pgdeactivate},
    {"pgfault", &vmem->vm_pgfault},
    {"pgfree", &vmem->vm_pgfree},
    {"pginodesteal", &vmem->vm_pginodesteal},
    {"pgmajfault", &vmem->vm_pgmajfault},
    {"pgpgin", &vmem->vm_pgpgin},	/* important */
    {"pgpgout", &vmem->vm_pgpgout},	/* important */
    {"pgrefill", &vmem->vm_pgrefill},	/* GONE (now separate dma,high,normal) */
    {"pgrefill_dma", &vmem->vm_pgrefill_dma},
    {"pgrefill_high", &vmem->vm_pgrefill_high},
    {"pgrefill_normal", &vmem->vm_pgrefill_normal},
    {"pgrotated", &vmem->vm_pgrotated},
    {"pgscan", &vmem->vm_pgscan},	/* GONE (now separate direct,kswapd and dma,high,normal) */
    {"pgscan_direct_dma", &vmem->vm_pgscan_direct_dma},
    {"pgscan_direct_high", &vmem->vm_pgscan_direct_high},
    {"pgscan_direct_normal", &vmem->vm_pgscan_direct_normal},
    {"pgscan_kswapd_dma", &vmem->vm_pgscan_kswapd_dma},
    {"pgscan_kswapd_high", &vmem->vm_pgscan_kswapd_high},
    {"pgscan_kswapd_normal", &vmem->vm_pgscan_kswapd_normal},
    {"pgsteal", &vmem->vm_pgsteal},	/* GONE (now separate dma,high,normal) */
    {"pgsteal_dma", &vmem->vm_pgsteal_dma},
    {"pgsteal_high", &vmem->vm_pgsteal_high},
    {"pgsteal_normal", &vmem->vm_pgsteal_normal},
    {"pswpin", &vmem->vm_pswpin},	/* important */
    {"pswpout", &vmem->vm_pswpout},	/* important */
    {"slabs_scanned", &vmem->vm_slabs_scanned},
  };
  const int vmem_table_count =
    sizeof (vmem_table) / sizeof (proc_table_struct);

  vmem->vm_pgalloc = 0;
  vmem->vm_pgrefill = 0;
  vmem->vm_pgscan = 0;
  vmem->vm_pgsteal = 0;

  procparser (PROC_VMSTAT, vmem_table, vmem_table_count, ' ');

  if (!vmem->vm_pgalloc)
    vmem->vm_pgalloc =
      vmem->vm_pgalloc_dma + vmem->vm_pgalloc_high + vmem->vm_pgalloc_normal;

  if (!vmem->vm_pgrefill)
    vmem->vm_pgrefill =
      vmem->vm_pgrefill_dma + vmem->vm_pgrefill_high +
      vmem->vm_pgrefill_normal;

  if (!vmem->vm_pgscan)
    vmem->vm_pgscan = vmem->vm_pgscan_direct_dma + vmem->vm_pgscan_direct_high
      + vmem->vm_pgscan_direct_normal + vmem->vm_pgscan_kswapd_dma
      + vmem->vm_pgscan_kswapd_high + vmem->vm_pgscan_kswapd_normal;

  if (!vmem->vm_pgsteal)
    vmem->vm_pgsteal =
      vmem->vm_pgsteal_dma + vmem->vm_pgsteal_high + vmem->vm_pgsteal_normal;
}

/* Drop a reference of the memory library context. If the refcount of
 * reaches zero, the resources of the context will be released.  */

struct proc_vmem *
proc_vmem_unref (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return NULL;

  vmem->refcount--;
  if (vmem->refcount > 0)
    return vmem;

  free (vmem);
  return NULL;
}

#define proc_vmem_get(arg) \
unsigned long proc_vmem_get_ ## arg (struct proc_vmem *p) \
  { return (p == NULL) ? 0 : p->vm_ ## arg; }

proc_vmem_get (pgalloc)
proc_vmem_get (pgfault)
proc_vmem_get (pgfree)
proc_vmem_get (pgmajfault)
proc_vmem_get (pgrefill)
proc_vmem_get (pgscan)
proc_vmem_get (pgsteal)
proc_vmem_get (pswpin)
proc_vmem_get (pswpout)

unsigned proc_vmem_get_pgpgin (struct proc_vmem *vmem)
{
  FILE *f;
  int need_vmstat_file = 1;
  unsigned long pgpgin, pgpgout;
  static char buf[2048];

  if (!(f = fopen (PROC_STAT, "r")))
    need_vmstat_file = 1;

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "page %lu %lu", &pgpgin, &pgpgout) == 2)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose (f);

  return need_vmstat_file ? vmem->vm_pgpgin : pgpgin;
}

unsigned proc_vmem_get_pgpgout (struct proc_vmem *vmem)
{
  FILE *f;
  int need_vmstat_file = 1;
  unsigned long pgpgin, pgpgout;
  static char buf[2048];

  if (!(f = fopen (PROC_STAT, "r")))
    need_vmstat_file = 1;

  while (fgets (buf, sizeof buf, f))
    {
      if (sscanf (buf, "page %lu %lu", &pgpgin, &pgpgout) == 2)
	{
	  need_vmstat_file = 0;
	  break;
	}
    }
  fclose (f);

  return need_vmstat_file ? vmem->vm_pgpgout : pgpgout;
}

unsigned long
proc_vmem_get_pgscand (struct proc_vmem *vmem)
{
  return (vmem ==
	  NULL) ? 0 : vmem->vm_pgscan_direct_dma +
    vmem->vm_pgscan_direct_high + vmem->vm_pgscan_direct_normal;
}

unsigned long
proc_vmem_get_pgscank (struct proc_vmem *vmem)
{
  return (vmem ==
	  NULL) ? 0 : vmem->vm_pgscan_kswapd_dma +
    vmem->vm_pgscan_kswapd_high + vmem->vm_pgscan_kswapd_normal;
}


/* get additional statistics for swap activity
 * Number of swapins and swapouts (since the last boot)	*/

void
proc_vmem_get_swap_io (unsigned long *pswpin, unsigned long *pswpout)
{
  FILE *f;
  int err, need_vmstat_file = 1;
  struct proc_vmem *vmem = NULL;
  static char buf[2048];

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
  fclose (f);

  if (need_vmstat_file)		/* Linux 2.5.40-bk4 and above */
    {
      err = proc_vmem_new (&vmem);
      if (err < 0)
	plugin_error (STATE_UNKNOWN, err, "memory exhausted");

      proc_vmem_read (vmem);

      *pswpin = proc_vmem_get_pswpin (vmem);
      *pswpout = proc_vmem_get_pswpout (vmem);

      proc_vmem_unref (vmem);
    }
}
