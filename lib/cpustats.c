/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the CPU utilization
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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpustats.h"
#include "messages.h"
#include "procparser.h"
#include "system.h"

#define PATH_PROC_STAT		"/proc/stat"

/* Fill the cpu_stats structure pointed with the values found in the
 * proc filesystem */

static void
cpu_stats_read (struct cpu_time *cputime,
		unsigned long long *nctxt,
		unsigned long long *nintr,
		unsigned long long *nsoftirq)
{
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  char *line = NULL;
  bool cputimes_found, intr_found, ctxt_found;

  if ((fp = fopen (PATH_PROC_STAT,  "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", PATH_PROC_STAT);

  if (nctxt) *nctxt = 0;
  if (nintr) *nintr = 0;
  if (nsoftirq) *nsoftirq = 0;
  cputimes_found = ctxt_found = intr_found = false;

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      if (!strncmp (line, "cpu ", 4))
	{
	  cputimes_found = true;
	  if (NULL == cputime)
	    continue;
	  sscanf (line, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
		  &cputime->user, &cputime->nice, &cputime->system,
		  &cputime->idle, &cputime->iowait, &cputime->irq,
		  &cputime->softirq, &cputime->steal, &cputime->guest,
		  &cputime->guestn);
	}
      else if (!strncmp (line, "ctxt ", 5))
	{
	  ctxt_found = true;
	  if (NULL == nctxt)
	    continue;
	  sscanf (line, "ctxt %Lu", nctxt);
	}
      /* Get the number of interrupts serviced since boot time, for each of the
       * possible system interrupts, including unnumbered architecture specific
       * interrupts  */
      else if (!strncmp (line, "intr ", 5))
	{
	  intr_found = true;
	  if (NULL == nintr)
	    continue;
	  sscanf (line, "intr %Lu", nintr);
	}
      /* Not separated out until the 2.6.0-test4 */
      else if (!strncmp (line, "softirq ", 8))
	{
	  if (NULL == nsoftirq)
	    continue;
	  sscanf (line, "softirq %Lu", nsoftirq);
	}
    }

  free (line);

  if (!cputimes_found || !ctxt_found || !intr_found)
    plugin_error (STATE_UNKNOWN, errno, "Error reading %s", PATH_PROC_STAT);
}

/* wrappers for cpu_stats_read () */

inline void
cpu_stats_get_time (struct cpu_time * __restrict cputime)
{
  cpu_stats_read (cputime, NULL, NULL, NULL);
}

inline void
cpu_stats_get_cswch (unsigned long long * __restrict nctxt)
{
  cpu_stats_read (NULL, nctxt, NULL, NULL);
}

inline void
cpu_stats_get_intr (unsigned long long * __restrict nintr)
{
  cpu_stats_read (NULL, NULL, nintr, NULL);
}

inline void
cpu_stats_get_softirq (unsigned long long * __restrict nsoftirq)
{
  cpu_stats_read (NULL, NULL, NULL, nsoftirq);
}