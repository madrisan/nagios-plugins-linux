/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
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

#define _GNU_SOURCE
#include <sched.h>

#include <sys/sysinfo.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sysfsparser.h"

#define PATH_SYS_SYSTEM		"/sys/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

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
  return sysfsparser_getvalue (PATH_SYS_CPU "/kernel_max") + 1;
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

/* Parses string with CPUs mask.  */

static int
cpumask_parse (const char *str, cpu_set_t *set, size_t setsize)
{
  int len = strlen (str);
  const char *ptr = str + len - 1;
  int cpu = 0;

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
      len--;
      ptr--;
      cpu += 4;
    }

  return 0;
}

/* Get the number of threads within one core */

unsigned int
get_cputopology_nthreads ()
{
  char *thread_siblings;
  cpu_set_t *set;
  size_t cpu, setsize = 0,
	 maxcpus = get_processor_number_kernel_max ();
  int rc;
  unsigned int nthreads = 0, curr;

  if (!(set = CPU_ALLOC (maxcpus)))
    return 0;
  setsize = CPU_ALLOC_SIZE (maxcpus);

  for (cpu = 0; cpu < maxcpus; cpu++)
    {
      /* thread_siblings. internal kernel map of cpu#'s hardware threads
       * within the same core as cpu#   */
      thread_siblings =
	sysfsparser_getline (PATH_SYS_CPU
			     "/cpu%u/topology/thread_siblings", cpu);
      if (!thread_siblings)
        continue;
      if ((rc = cpumask_parse (thread_siblings, set, setsize)))
	continue;

      curr = CPU_COUNT_S (setsize, set);
      if (nthreads < curr)	/* not sure this makes sense.. */
	nthreads = curr;	/* 'curr' should be the same on each cpu# */
      free (thread_siblings);
    }

  CPU_FREE (set);
  return nthreads;
}
