/* cputopology.h -- a library for checking the CPU topology

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _CPUTOPOLOGY_H
#define _CPUTOPOLOGY_H

#include <sched.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /*
   * Fallback for old or obscure libcs without dynamically allocated cpusets
   *
   * The following macros are based on code from glibc.
   *
   * The GNU C Library is free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public
   * License as published by the Free Software Foundation; either
   * version 2.1 of the License, or (at your option) any later version.
   */
  #if !HAVE_DECL_CPU_ALLOC

  # define CPU_ZERO_S(setsize, cpusetp)                              \
    do {                                                             \
      size_t __i;                                                    \
      size_t __imax = (setsize) / sizeof (__cpu_mask);               \
      __cpu_mask *__bits = (cpusetp)->__bits;                        \
      for (__i = 0; __i < __imax; ++__i)                             \
        __bits[__i] = 0;                                             \
    } while (0)

  # define CPU_SET_S(cpu, setsize, cpusetp)                          \
     ({ size_t __cpu = (cpu);                                        \
        __cpu < 8 * (setsize)                                        \
        ? (((__cpu_mask *) ((cpusetp)->__bits))[__CPUELT (__cpu)]    \
           |= __CPUMASK (__cpu))                                     \
        : 0; })

  int __cpuset_count_s(size_t setsize, const cpu_set_t *set);
  # define CPU_COUNT_S(setsize, cpusetp)  __cpuset_count_s(setsize, cpusetp)

  # define CPU_ALLOC_SIZE(count) \
	   ((((count) + __NCPUBITS - 1) / __NCPUBITS) * sizeof (__cpu_mask))
  # define CPU_ALLOC(count)	(malloc(CPU_ALLOC_SIZE(count)))
  # define CPU_FREE(cpuset)	(free(cpuset))

  #endif	/* !HAVE_DECL_CPU_ALLOC */

  /* Get the number of total and active cpus. */
  int get_processor_number_total ();
  int get_processor_number_online ();

  /* Get the maximum cpu index allowed by the kernel configuration. */
  int get_processor_number_kernel_max ();

  /* Get the number of sockets, cores, and threads. */
  int get_cputopology_nthreads ();
  void get_cputopology_read (unsigned int *nsockets, unsigned int *ncores,
			     unsigned int *nthreads);

#ifdef __cplusplus
}
#endif

#endif		/* _CPUTOPOLOGY_H */
