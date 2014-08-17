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
#include <stdlib.h>

#include "logging.h"
#include "messages.h"
#include "meminfo.h"
#include "procparser.h"
#include "system.h"

/*#define PROC_MEMINFO  "/proc/meminfo"*/
#define MEMINFO_UNSET ~0UL

typedef struct proc_sysmem_data
{
  /* 3.14+
   * MemAvailable provides an estimate of how much memory is available for
   * starting new applications, without swapping.
   * However, unlike the data provided by the Cache or Free fields,
   * MemAvailable takes into account page cache and also that not all
   * reclaimable memory slabs will be reclaimable due to items being in
   * use.
   * See: https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/\
   *  commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
   *
   * FIXME: A fallback MemAvailable evaluation for 2.6.27 <= kernels < 3.14
   * can be implemented.
   * See: https://gitorious.org/procps/procps/commit/\
   *  b779855cf15d68f9038ff1809db18c0788e9ae70.patch 
   */
  bool native_memavailable;
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
  /* 3.14+ */
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
    { "AnonPages", &data->kb_anon_pages },
    { "Bounce", &data->kb_bounce },
    { "Buffers", &data->kb_main_buffers }, /* important */
    { "Cached", &data->kb_page_cache },   /* important */
    { "CommitLimit", &data->kb_commit_limit },
    { "Committed_AS", &data->kb_committed_as },
    { "Dirty", &data->kb_dirty },  /* kB version of vmstat nr_dirty */
    { "HighFree", &data->kb_high_free },
    { "HighTotal", &data->kb_high_total },
    { "Inact_clean", &data->kb_inact_clean },
    { "Inact_dirty", &data->kb_inact_dirty },
    { "Inact_laundry", &data->kb_inact_laundry },
    { "Inact_target", &data->kb_inact_target },
    { "Inactive", &data->kb_inactive },	 /* important */
    { "LowFree", &data->kb_low_free },
    { "LowTotal", &data->kb_low_total },
    { "Mapped", &data->kb_mapped },        /* kB version of vmstat nr_mapped */
    { "MemAvailable", &data->kb_main_available },  /* kernel 3.14 and later */
    { "MemFree", &data->kb_main_free },	    /* important */
    { "MemTotal", &data->kb_main_total },    /* important */
    { "NFS_Unstable", &data->kb_nfs_unstable },
    { "PageTables", &data->kb_pagetables },   /* kB version of vmstat nr_page_table_pages */
    { "ReverseMaps", &data->nr_reversemaps }, /* same as vmstat nr_page_table_pages */
    { "SReclaimable", &data->kb_swap_reclaimable },  /* "swap reclaimable" (dentry and inode structures) */
    { "SUnreclaim", &data->kb_swap_unreclaimable },
    { "Shmem", &data->kb_main_shared},        /* kernel 2.6.32 and later */
    { "Slab", &data->kb_slab },               /* kB version of vmstat nr_slab */
    { "SwapCached", &data->kb_swap_cached },  /* late 2.4 and 2.6+ only */
    { "SwapFree", &data->kb_swap_free },      /* important */
    { "SwapTotal", &data->kb_swap_total },    /* important */
    { "VmallocChunk", &data->kb_vmalloc_chunk },
    { "VmallocTotal", &data->kb_vmalloc_total },
    { "VmallocUsed", &data->kb_vmalloc_used },
    { "Writeback", &data->kb_writeback },     /* kB version of vmstat nr_writeback */
  };
  const int sysmem_table_count = sizeof (sysmem_table) / sizeof (proc_table_struct);

  data->kb_inactive = MEMINFO_UNSET;
  data->kb_low_total = MEMINFO_UNSET;
  data->kb_main_available = MEMINFO_UNSET;

  procparser (PROC_MEMINFO, sysmem_table, sysmem_table_count, ':');

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

  if (data->kb_main_available == MEMINFO_UNSET)
    {
      /* FIXME - implement the fallback for 2.6.27 <= kernel < 3.14 */
      dbg ("MemAvailable is not provided by /proc/meminfo...\n");
      dbg ("...falling back to MemFree\n");
      data->kb_main_available = data->kb_main_free;
      data->native_memavailable = false;
    }
  else
    data->native_memavailable = true;
  
  /* derived values */
  data->kb_main_cached = data->kb_page_cache + data->kb_slab;
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
