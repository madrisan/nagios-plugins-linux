/* Library for getting system memory informations

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

  enum unit_shift
  {
    b_shift = 0, k_shift = 10, m_shift = 20, g_shift = 30
  };

#define SU(X) ( ((unsigned long long)(X) << k_shift) >> shift ), units

  struct proc_sysmem;

  /* Allocates space for a new sysmem object.
   * Returns 0 if all went ok. Errors are returned as negative values.  */
  int proc_sysmem_new (struct proc_sysmem **sysmem);

  /* Fill the proc_sysmem structure pointed with the values found in the
   * proc filesystem.  */
  void proc_sysmem_read (struct proc_sysmem *sysmem);

  /* Drop a reference of the memory library context. If the refcount of
   * reaches zero, the resources of the context will be released.  */
  struct proc_sysmem *proc_sysmem_unref (struct proc_sysmem *sysmem);

  /* Accessing the values from proc_sysmem */

  unsigned long proc_sysmem_get_active (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_anon_pages (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_committed_as (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_dirty (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_inactive (struct proc_sysmem *sysmem);

  unsigned long proc_sysmem_get_main_available (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_buffers (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_cached (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_free (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_shared (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_total (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_main_used (struct proc_sysmem *sysmem);

  /* return true if the kernel provides the counter 'MemAvailable' (3.14+) */
  bool proc_sysmem_native_memavailable (struct proc_sysmem *sysmem);

  unsigned long proc_sysmem_get_swap_cached (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_swap_free (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_swap_total (struct proc_sysmem *sysmem);
  unsigned long proc_sysmem_get_swap_used (struct proc_sysmem *sysmem);

#ifdef __cplusplus
}
#endif

#endif				/* _MEMINFO_H_ */
