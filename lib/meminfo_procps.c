// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2017,2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking memory and swap usage on linux.
 * This library is a front-end for `procps-newlib':
 *      https://gitlab.com/madrisan/procps/tree/newlib
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
 */

#include <errno.h>
#include <stdlib.h>
#include <libproc2/meminfo.h>

typedef struct proc_sysmem
{
  int refcount;
  struct meminfo_info *info;	/* proc/meminfo.{h,c} */
} proc_sysmem_t;

int
proc_sysmem_new (struct proc_sysmem **sysmem)
{
  struct proc_sysmem *m;

  m = calloc (1, sizeof (struct proc_sysmem));
  if (!m)
    return -ENOMEM;

  m->refcount = 1;

  if (procps_meminfo_new (&m->info) < 0)
    return -ENOMEM;

  *sysmem = m;
  return 0;
}

void
proc_sysmem_read (struct proc_sysmem *sysmem __attribute__ ((unused)))
{
}

struct proc_sysmem *
proc_sysmem_unref (struct proc_sysmem *sysmem)
{
  if (sysmem == NULL)
    return NULL;

  sysmem->refcount--;
  if (sysmem->refcount > 0)
    return sysmem;

  procps_meminfo_unref (&(sysmem->info));
  free (sysmem);
  return NULL;
}

#define proc_sysmem_get(arg, item) \
unsigned long proc_sysmem_get_ ## arg (struct proc_sysmem *p) \
  { return (p == NULL) ? 0 : MEMINFO_GET (p->info, item, ul_int); }

/* Memory that has been used more recently and usually not reclaimed unless
 * absolutely necessary.  */
proc_sysmem_get(active, MEMINFO_MEM_ACTIVE)
/* Non-file backed pages mapped into user-space page tables.
 * High AnonPages means too much memory been allocated (mostly by malloc call)
 * but not released yet (by instance because of memory leaks - process(es)
 * that allocate but never free memory.  */
proc_sysmem_get(anon_pages, MEMINFO_MEM_ANON)
/* The amount of memory presently allocated on the system. */
proc_sysmem_get(committed_as, MEMINFO_MEM_COMMITTED_AS)
/* Memory which is waiting to get written back to the disk. */
proc_sysmem_get(dirty, MEMINFO_MEM_DIRTY)
/* Memory which has been less recently used.
 * Inactive memory is the best candidate for reclaiming memory and so low
 * inactive memory would mean that you are low on memory and the kernel may
 + have to swap out process pages, or swap out the cache to disk or in the
 * worst case if it runs out of swap space then begin killing processes.  */
proc_sysmem_get(inactive, MEMINFO_MEM_INACTIVE)
proc_sysmem_get(main_available, MEMINFO_MEM_AVAILABLE)
proc_sysmem_get(main_buffers, MEMINFO_MEM_BUFFERS)
proc_sysmem_get(main_cached, MEMINFO_MEM_CACHED_ALL)
proc_sysmem_get(main_free, MEMINFO_MEM_FREE)
proc_sysmem_get(main_shared, MEMINFO_MEM_SHARED)
proc_sysmem_get(main_total, MEMINFO_MEM_TOTAL)
proc_sysmem_get(main_used, MEMINFO_MEM_USED)

proc_sysmem_get(swap_cached, MEMINFO_SWAP_CACHED)
proc_sysmem_get(swap_free, MEMINFO_SWAP_FREE)
proc_sysmem_get(swap_total, MEMINFO_SWAP_TOTAL)

unsigned long
proc_sysmem_get_swap_used (struct proc_sysmem *sysmem)
{
  return (sysmem == NULL) ? 0 :
    proc_sysmem_get_swap_total (sysmem) - proc_sysmem_get_swap_free (sysmem);
}

#undef proc_sysmem_get
