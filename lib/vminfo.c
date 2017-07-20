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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "system.h"
#include "getenv.h"
#include "messages.h"
#include "meminfo.h"
#include "procparser.h"

#define PROC_STAT     "/proc/stat"

const char *
get_path_proc_vmstat ()
{
  const char *env_procvmstat = secure_getenv ("NPL_TEST_PATH_PROCVMSTAT");
  if (env_procvmstat)
    return env_procvmstat;

  return "/proc/vmstat";
}

/* get_vmem_pagesize - get memory page size */

inline long
get_vmem_pagesize (void)
{
  return sysconf (_SC_PAGESIZE);
}

typedef struct proc_vmem_data
{
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
} proc_vmem_data_t;

typedef struct proc_vmem
{
  int refcount;
  struct proc_vmem_data *data;
} proc_vmem_t;

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
  vm->data = calloc (1, sizeof (struct proc_vmem_data));
  if (!vm->data)
    {
      free (vm);
      return -ENOMEM;
    }

  *vmem = vm;
  return 0;
}

/* Fill the proc_vmem structure pointed will the values found in the
 * proc filesystem. */

void
proc_vmem_read (struct proc_vmem *vmem)
{
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  bool found_pgpg_data = false, found_pswp_data = false;

  if (vmem == NULL)
    return;

  struct proc_vmem_data *data = vmem->data;

  const proc_table_struct vmem_table[] = {
    {"allocstall", &data->vm_allocstall},
    {"kswapd_inodesteal", &data->vm_kswapd_inodesteal},
    {"kswapd_steal", &data->vm_kswapd_steal},
    {"nr_dirty", &data->vm_nr_dirty},	/* page version of meminfo Dirty */
    {"nr_mapped", &data->vm_nr_mapped},	/* page version of meminfo Mapped */
    {"nr_page_table_pages", &data->vm_nr_page_table_pages},	/* same as meminfo PageTables */
    {"nr_pagecache", &data->vm_nr_pagecache},	/* gone in 2.5.66+ kernels */
    {"nr_reverse_maps", &data->vm_nr_reverse_maps},	/* page version of meminfo ReverseMaps GONE */
    {"nr_slab", &data->vm_nr_slab},	/* page version of meminfo Slab */
    {"nr_unstable", &data->vm_nr_unstable},
    {"nr_writeback", &data->vm_nr_writeback},	/* page version of meminfo Writeback */
    {"pageoutrun", &data->vm_pageoutrun},
    {"pgactivate", &data->vm_pgactivate},
    {"pgalloc", &data->vm_pgalloc},	/* GONE (now separate dma,high,normal) */
    {"pgalloc_dma", &data->vm_pgalloc_dma},
    {"pgalloc_high", &data->vm_pgalloc_high},
    {"pgalloc_normal", &data->vm_pgalloc_normal},
    {"pgdeactivate", &data->vm_pgdeactivate},
    {"pgfault", &data->vm_pgfault},
    {"pgfree", &data->vm_pgfree},
    {"pginodesteal", &data->vm_pginodesteal},
    {"pgmajfault", &data->vm_pgmajfault},
    {"pgpgin", &data->vm_pgpgin},	/* important */
    {"pgpgout", &data->vm_pgpgout},	/* important */
    {"pgrefill", &data->vm_pgrefill},	/* GONE (now separate dma,high,normal) */
    {"pgrefill_dma", &data->vm_pgrefill_dma},
    {"pgrefill_high", &data->vm_pgrefill_high},
    {"pgrefill_normal", &data->vm_pgrefill_normal},
    {"pgrotated", &data->vm_pgrotated},
    {"pgscan", &data->vm_pgscan},	/* GONE (now separate direct,kswapd and dma,high,normal) */
    {"pgscan_direct_dma", &data->vm_pgscan_direct_dma},
    {"pgscan_direct_high", &data->vm_pgscan_direct_high},
    {"pgscan_direct_normal", &data->vm_pgscan_direct_normal},
    {"pgscan_kswapd_dma", &data->vm_pgscan_kswapd_dma},
    {"pgscan_kswapd_high", &data->vm_pgscan_kswapd_high},
    {"pgscan_kswapd_normal", &data->vm_pgscan_kswapd_normal},
    {"pgsteal", &data->vm_pgsteal},	/* GONE (now separate dma,high,normal) */
    {"pgsteal_dma", &data->vm_pgsteal_dma},
    {"pgsteal_high", &data->vm_pgsteal_high},
    {"pgsteal_normal", &data->vm_pgsteal_normal},
    {"pswpin", &data->vm_pswpin},	/* important */
    {"pswpout", &data->vm_pswpout},	/* important */
    {"slabs_scanned", &data->vm_slabs_scanned},
  };
  const int vmem_table_count =
    sizeof (vmem_table) / sizeof (proc_table_struct);

  data->vm_pgalloc = 0;
  data->vm_pgrefill = 0;
  data->vm_pgscan = 0;
  data->vm_pgsteal = 0;

  data->vm_pgpgin = data->vm_pgpgout = ~0UL;
  data->vm_pswpin = data->vm_pswpout = ~0UL;

  procparser (get_path_proc_vmstat (), vmem_table, vmem_table_count, ' ');

  if (!data->vm_pgalloc)
    data->vm_pgalloc =
      data->vm_pgalloc_dma + data->vm_pgalloc_high + data->vm_pgalloc_normal;

  if (!data->vm_pgrefill)
    data->vm_pgrefill =
      data->vm_pgrefill_dma + data->vm_pgrefill_high +
      data->vm_pgrefill_normal;

  if (!data->vm_pgscan)
    data->vm_pgscan = data->vm_pgscan_direct_dma + data->vm_pgscan_direct_high
      + data->vm_pgscan_direct_normal + data->vm_pgscan_kswapd_dma
      + data->vm_pgscan_kswapd_high + data->vm_pgscan_kswapd_normal;

  if (!data->vm_pgsteal)
    data->vm_pgsteal =
      data->vm_pgsteal_dma + data->vm_pgsteal_high + data->vm_pgsteal_normal;

  if (data->vm_pgpgin != ~0UL && data->vm_pswpin != ~0UL)
    return;
  else if (data->vm_pgpgin != ~0UL)
    found_pgpg_data = true;
  else if (data->vm_pswpin != ~0UL)
    found_pswp_data = true;

  /* Linux kernels < 2.5.40-bk4 */

  if ((fp = fopen (PROC_STAT, "r")))
    {
      while ((nread = getline (&line, &len, fp)) != -1)
        {
          if (2 == sscanf (line, "page %lu %lu",
                           &data->vm_pgpgin, &data->vm_pgpgout))
            found_pgpg_data = true;
          else if (2 == sscanf (line, "swap %lu %lu",
                                &data->vm_pswpin, &data->vm_pswpout))
            found_pswp_data = true;

          if (found_pgpg_data && found_pswp_data)
            break;
        }
      fclose (fp);
      free (line);
    }

    /* This should never occur */
    if (!found_pgpg_data)
      data->vm_pgpgin = data->vm_pgpgout = 0;
    if (!found_pswp_data)
      data->vm_pswpin = data->vm_pswpout = 0;
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

  free (vmem->data);
  free (vmem);
  return NULL;
}

#define proc_vmem_get(arg) \
unsigned long proc_vmem_get_ ## arg (struct proc_vmem *p) \
  { return (p == NULL) ? 0 : p->data->vm_ ## arg; }

proc_vmem_get (pgalloc)
proc_vmem_get (pgfault)
proc_vmem_get (pgfree)
proc_vmem_get (pgmajfault)
proc_vmem_get (pgpgin)
proc_vmem_get (pgpgout)
proc_vmem_get (pgrefill)
proc_vmem_get (pgscan)
proc_vmem_get (pgsteal)
proc_vmem_get (pswpin)
proc_vmem_get (pswpout)

#undef proc_vmem_get

unsigned long
proc_vmem_get_pgscand (struct proc_vmem *vmem)
{
  return (vmem ==
	  NULL) ? 0 : vmem->data->vm_pgscan_direct_dma +
    vmem->data->vm_pgscan_direct_high + vmem->data->vm_pgscan_direct_normal;
}

unsigned long
proc_vmem_get_pgscank (struct proc_vmem *vmem)
{
  return (vmem ==
	  NULL) ? 0 : vmem->data->vm_pgscan_kswapd_dma +
    vmem->data->vm_pgscan_kswapd_high + vmem->data->vm_pgscan_kswapd_normal;
}
