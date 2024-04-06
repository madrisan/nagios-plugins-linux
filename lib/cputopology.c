// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014-2015,2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the CPU topology
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

/* A note on terminology
 * ---------------------
 * CPU    The logical CPU number of a CPU as used by the Linux kernel.
 * CORE   The logical core number.  A core can contain several CPUs.
 * SOCKET The logical socket number.  A socket can contain several cores.
 * BOOK   The logical book number.  A book can contain several sockets.
 * NODE   The logical NUMA node number.  A node may contain several books.  */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <sys/sysinfo.h>

#include <ctype.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "cputopology.h"
#include "sysfsparser.h"

#define PATH_SYS_SYSTEM		PATH_SYS "/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

#if !HAVE_DECL_CPU_ALLOC
/* Please, use CPU_COUNT_S() macro. This is fallback */
int
__cpuset_count_s (size_t setsize, const cpu_set_t * set)
{
  int s = 0;
  const __cpu_mask *p = set->__bits;
  const __cpu_mask *end = &set->__bits[setsize / sizeof (__cpu_mask)];

  while (p < end)
    {
      __cpu_mask l = *p++;

      if (l == 0)
	continue;
#if LONG_BIT > 32
      l = (l & 0x5555555555555555ul) + ((l >> 1) & 0x5555555555555555ul);
      l = (l & 0x3333333333333333ul) + ((l >> 2) & 0x3333333333333333ul);
      l = (l & 0x0f0f0f0f0f0f0f0ful) + ((l >> 4) & 0x0f0f0f0f0f0f0f0ful);
      l = (l & 0x00ff00ff00ff00fful) + ((l >> 8) & 0x00ff00ff00ff00fful);
      l = (l & 0x0000ffff0000fffful) + ((l >> 16) & 0x0000ffff0000fffful);
      l = (l & 0x00000000fffffffful) + ((l >> 32) & 0x00000000fffffffful);
#else
      l = (l & 0x55555555ul) + ((l >> 1) & 0x55555555ul);
      l = (l & 0x33333333ul) + ((l >> 2) & 0x33333333ul);
      l = (l & 0x0f0f0f0ful) + ((l >> 4) & 0x0f0f0f0ful);
      l = (l & 0x00ff00fful) + ((l >> 8) & 0x00ff00fful);
      l = (l & 0x0000fffful) + ((l >> 16) & 0x0000fffful);
#endif
      s += l;
    }
  return s;
}
#endif

/* Get the number of total and active cpus */

/* Note! On Linux systems with glibc,
 *   sysconf (_SC_NPROCESSORS_CONF)
 *   sysconf (_SC_NPROCESSORS_ONLN)
 * come from the /sys and /proc file systems (see
 * glibc/sysdeps/unix/sysv/linux/getsysstats.c).
 * In some situations these file systems are not mounted, and the sysconf
 * call returns 1, which does not reflect the reality.   */

int
get_processor_number_total (void)
{
  /* The number of CPUs configured in the system.   */
  return
#if defined (HAVE_GET_NPROCS_CONF)
    get_nprocs_conf ();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_CONF)
    sysconf (_SC_NPROCESSORS_CONF);
#else
    -1;
#endif
}

int
get_processor_number_online (void)
{
  /* The number of CPUs available to the scheduler.   */
  return
#if defined (HAVE_GET_NPROCS)
    get_nprocs ();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_ONLN)
    sysconf (_SC_NPROCESSORS_ONLN);
#else
    -1;
#endif
}

/* Get the maximum cpu index allowed by the kernel configuration. */

int
get_processor_number_kernel_max ()
{
  unsigned long long value;

  sysfsparser_getvalue (&value, PATH_SYS_CPU "/kernel_max");
  return value + 1;
}

static inline int
char_to_val (int c)
{
  int cl;

  cl = tolower (c);
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (cl >= 'a' && cl <= 'f')
    return cl + (10 - 'a');
  else
    return -1;
}

/* Parses string with CPUs mask and return the number of CPUs present. */

static int
cpumask_parse (const char *str, cpu_set_t *set, size_t setsize)
{
  const char *ptr;
  int len, cpu = 0;

  if (!str)
    return -1;

  len = strlen (str);
  ptr = str + len - 1;

  /* skip 0x, it's all hex anyway */
  if (len > 1 && !memcmp (str, "0x", 2L))
    str += 2;

  CPU_ZERO_S (setsize, set);

  while (ptr >= str)
    {
      char val;

      /* cpu masks in /sys uses comma as a separator */
      if (*ptr == ',')
	ptr--;

      val = char_to_val (*ptr);
      if (val == (char) -1)
	return -1;
      if (val & 1)
	CPU_SET_S (cpu, setsize, set);
      if (val & 2)
	CPU_SET_S (cpu + 1, setsize, set);
      if (val & 4)
	CPU_SET_S (cpu + 2, setsize, set);
      if (val & 8)
	CPU_SET_S (cpu + 3, setsize, set);
      ptr--;
      cpu += 4;
    }

  return CPU_COUNT_S (setsize, set);
}

/* Get the number of threads within one core */

void
get_cputopology_read (unsigned int *nsockets, unsigned int *ncores,
		      unsigned int *nthreads)
{
  cpu_set_t *set;
  size_t cpu,	/* most 32 bit architectures use "unsigned int" size_t,
		 * and all 64 bit architectures use "unsigned long" size_t */
	 maxcpus, setsize = 0;

  *nsockets = *ncores = *nthreads = 1;
  maxcpus = get_processor_number_kernel_max ();

  if (!(set = CPU_ALLOC (maxcpus)))
    return;
  setsize = CPU_ALLOC_SIZE (maxcpus);

  for (cpu = 0; cpu < maxcpus; cpu++)
    {
      /* thread_siblings: internal kernel map of cpu#'s hardware threads
       * within the same core as cpu#   */
      char *thread_siblings =
	sysfsparser_getline (PATH_SYS_CPU "/cpu%lu/topology/thread_siblings",
			     (unsigned long)cpu);
      if (!thread_siblings)
        continue;

      /* threads within one core */
      *nthreads = cpumask_parse (thread_siblings, set, setsize);
      if (*nthreads == 0)
	*nthreads = 1;

      /* core_siblings: internal kernel map of cpu#'s hardware threads
       * within the same physical_package_id.  */
      char *core_siblings =
	sysfsparser_getline (PATH_SYS_CPU "/cpu%lu/topology/core_siblings",
			     (unsigned long)cpu);
      /* cores within one socket */
      *ncores = cpumask_parse (core_siblings, set, setsize) / *nthreads;
      if (*ncores == 0)
	*ncores = 1;

      int ncpus_online = get_processor_number_online ();
      unsigned int ncpus = ncpus_online > 0 ? ncpus_online : 0;

      *nsockets = ncpus / *nthreads / *ncores;
      if (!*nsockets)
	*nsockets = 1;

      free (core_siblings);
      free (thread_siblings);
    }

  CPU_FREE (set);
}
