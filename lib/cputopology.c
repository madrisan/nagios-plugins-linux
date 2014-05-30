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

static const char *
nexttoken (const char *q, int sep)
{
  if (q)
    q = strchr (q, sep);
  if (q)
    q++;
  return q;
}

/* Parses string with list of CPU ranges.
 * Returns 1 on error.  */

static int
cpulist_parse (const char *str, cpu_set_t *set, size_t setsize)
{
  const char *p, *q;
  int r = 0;

  CPU_ZERO_S (setsize, set);

  q = str;
  while (p = q, q = nexttoken (q, ','), p)
    {
      unsigned int a; /* beginning of range */
      unsigned int b; /* end of range */
      unsigned int s; /* stride */
      const char *c1, *c2;
      char c;

      if ((r = sscanf (p, "%u%c", &a, &c)) < 1)
	return 1;

      b = a;
      s = 1;

      c1 = nexttoken (p, '-');
      c2 = nexttoken (p, ',');
      if (c1 != NULL && (c2 == NULL || c1 < c2))
	{
	  if ((r = sscanf (c1, "%u%c", &b, &c)) < 1)
	    return 1;

	  c1 = nexttoken (c1, ':');
	  if (c1 != NULL && (c2 == NULL || c1 < c2))
	    {
	      if ((r = sscanf (c1, "%u%c", &s, &c)) < 1)
		return 1;
	      if (s == 0)
		return 1;
	    }
	}

      if (!(a <= b))
	return 1;

      while (a <= b)
	{
	  CPU_SET_S (a, setsize, set);
	  a += s;
	}
    }

  if (r == 2)
    return 1;

  return 0;
}

/* Get the number of threads within one core */
unsigned int
get_processor_nthreads ()
{
  char *list;
  cpu_set_t *set;
  size_t nthreads = 0, setsize = 0,
	 maxcpus = get_processor_number_kernel_max ();
  int rc;

  list = sysfsparser_getline (PATH_SYS_CPU "/online");
  if (NULL == list)
    return 0;

  if (NULL == (set = CPU_ALLOC (maxcpus)))
    return 0;

  setsize = CPU_ALLOC_SIZE (maxcpus);

  if ((rc = cpulist_parse (list, set, setsize)))
    return 0;

  nthreads = CPU_COUNT_S (setsize, set);

  CPU_FREE (set);
  free (list);

  return nthreads;
}
