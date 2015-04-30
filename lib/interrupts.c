/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for getting informations on the system interrupts
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "cputopology.h"
#include "logging.h"
#include "system.h"
#include "xalloc.h"

#define PROC_ROOT	"/proc"
#define PROC_INTR	PROC_ROOT "/interrupts"

/* Return an array containing the number of interrupts per cpu per IO device.
 * Since Linux 2.6.24, for the i386 and x86_64 architectures at least,
 * /proc/interrupts also includes interrupts internal to the system (that is,
 * not associated with a device as such).  */
unsigned long *
proc_interrupts_get_nintr_per_cpu (unsigned int *ncpus)
{
  FILE *fp;
  char *p, *end, *line = NULL;
  size_t len = 0;
  ssize_t chread;
  bool header = true;
  unsigned int cpu;

  if ((fp = fopen (PROC_INTR, "r")) == NULL)
    return NULL;

  *ncpus = get_processor_number_online ();
  unsigned long value,
		*vintr = xnmalloc (*ncpus, sizeof (unsigned long));

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      /* skip the first line */
      if (header)
	{
	  header = false;
	  continue;
	}

      p = strchr(line, ':');
      if (NULL == p)	/* this should never happen */
	continue;

      dbg ("%s", line);
      for (cpu = 0; cpu < *ncpus; cpu++)
	{
	  if (*p == '\0' || *p == '\n')
	    continue;
	  value = strtoul (p + 1, &end, 10);
	  dbg (" --> cpu%d = %lu\n", cpu, value);
	  vintr[cpu] += value;
	  p = end + 1;
	} 
      dbg ("\n");
    }

  free (line);

  return vintr;
}
