// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2017,2024 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking memory and swap usage on linux.
 * This library is a front-end for `procps-newlib':
 * 	https://gitlab.com/madrisan/procps/tree/newlib
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
#include <libproc2/vmstat.h>

typedef struct proc_vmem
{
  int refcount;
  struct vmstat_info *info;	/* proc/vmstat.h */
} proc_vmem_t;

int
proc_vmem_new (struct proc_vmem **vmem)
{
  struct proc_vmem *vm;

  vm = calloc (1, sizeof (struct proc_vmem));
  if (!vm)
    return -ENOMEM;

  vm->refcount = 1;

  if (procps_vmstat_new (&vm->info) < 0)
    return -ENOMEM;

  *vmem = vm;
  return 0;
}

void
proc_vmem_read (struct proc_vmem *vmem __attribute__ ((unused)))
{
}

struct proc_vmem *
proc_vmem_unref (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return NULL;

  vmem->refcount--;
  if (vmem->refcount > 0)
    return vmem;

  procps_vmstat_unref (&(vmem->info));
  free (vmem);
  return NULL;
}

#define proc_vmem_get(arg, item) \
unsigned long proc_vmem_get_ ## arg (struct proc_vmem *p) \
  { return (p == NULL) ? 0 : VMSTAT_GET(p->info, item, ul_int); }

proc_vmem_get (pgfault, VMSTAT_PGFAULT)
proc_vmem_get (pgfree, VMSTAT_PGFREE)
proc_vmem_get (pgmajfault, VMSTAT_PGMAJFAULT)
proc_vmem_get (pgpgin, VMSTAT_PGPGIN) proc_vmem_get (pgpgout, VMSTAT_PGPGOUT)
proc_vmem_get (pswpin, VMSTAT_PSWPIN)
proc_vmem_get (pswpout, VMSTAT_PSWPOUT)

#undef proc_vmem_get

/*
 * DMA memory zone:
 *  - low 16MB of memory
 *  - exists for historical reasons, sometime there was hardware that
 *    could only do DMA in this area
 * DMA32 memory zone:
 *  - only in 64-bit linux
 *  - low ~4GBytes of memory
 *  - today, there is hardware that can do DMA to 4GBytes
 * Normal memory zone:
 *  different on 32-bit and 64-bit machines
 *    - 32-bit: Memory from 16MB to 896MB
 *    - 64-bit: Memory above ~4GB
 * HighMem memory zone:
 *  - only on 32-bit Linux
 *  - all Memory above ~896 MB
 *  - is not permanently or automatically mapped into the kernel’s
 *    address space
 *
 * cat /proc/pagetypeinfo
 */

#define GET_DATA(item) VMSTAT_GET(vmem->info, item, ul_int)

unsigned long
proc_vmem_get_pgsteal (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return 0;

  return GET_DATA (VMSTAT_PGSTEAL_DIRECT);
}

unsigned long
proc_vmem_get_pgscand (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return 0;

  return GET_DATA (VMSTAT_PGSCAN_DIRECT);
}

unsigned long
proc_vmem_get_pgscank (struct proc_vmem *vmem)
{
  if (vmem == NULL)
    return 0;

  return GET_DATA (VMSTAT_PGSCAN_KSWAPD);
}

#undef GET_DATA
