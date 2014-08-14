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

/* note! This buffer may not be big enough if lots of CPUs are online */
#define BUFFSIZE 0x1000		/* 4kB */
static char buff[BUFFSIZE];

#define PATH_PROC_STAT		"/proc/stat"

/* Fill the cpu_stats structure pointed with the values found in the
 * proc filesystem */

static void
cpu_stats_read (struct cpu_time *cputime,
		unsigned long long *nctxt,
		unsigned long long *nintr,
		unsigned long long *nsoftirq)
{
  static int fd;
  const char *b;

  if (fd)
    lseek (fd, 0L, SEEK_SET);
  else
    {
      fd = open (PATH_PROC_STAT, O_RDONLY, 0);
      if (fd == -1)
	plugin_error (STATE_UNKNOWN, errno, "Error opening %s",
		      PATH_PROC_STAT);
    }

  read (fd, buff, BUFFSIZE - 1);

  if (nctxt) *nctxt = 0;
  if (nintr) *nintr = 0;
  if (nsoftirq) *nsoftirq = 0;

  if (cputime)
    {
      if ((b = strstr (buff, "cpu ")))
	sscanf (b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
		&cputime->user, &cputime->nice, &cputime->system,
		&cputime->idle, &cputime->iowait, &cputime->irq,
		&cputime->softirq, &cputime->steal, &cputime->guest,
		&cputime->guestn);
      else
	goto readerr;
    }

  if (nctxt)
    {
      if ((b = strstr (buff, "ctxt ")))
	sscanf (b, "ctxt %Lu", nctxt);
      else
	goto readerr;
    }

  /* Get the number of interrupts serviced since boot time, for each of the
   * possible system interrupts, including unnumbered architecture specific
   * interrupts  */
  if (nintr)
    {
      if ((b = strstr (buff, "intr ")))
	sscanf (b, "intr %Lu ", nintr);
      else
	goto readerr;
    }

  /* Not separated out until the 2.6.0-test4 */
  if (nsoftirq)
    if ((b = strstr (buff, "softirq ")))
      sscanf (b, "softintr %Lu ", nsoftirq);

  return;

readerr:
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