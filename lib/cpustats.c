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

void
cpu_stats_read (struct cpu_stats *cpustats)
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

  cpustats->iowait = 0;	/* not separated out until the 2.5.41 kernel */
  cpustats->irq = 0;	/* not separated out until the 2.6.0-test4 */
  cpustats->softirq = 0;	/* not separated out until the 2.6.0-test4 */
  cpustats->steal = 0;	/* not separated out until the 2.6.11 */
  cpustats->guest = 0;	/* since Linux 2.6.24 */
  cpustats->guestn = 0;	/* since Linux 2.6.33 */

  cpustats->nctxt = 0;
  cpustats->nintr = 0;
  cpustats->nsoftirq = 0;

  if ((b = strstr (buff, "cpu ")))
    sscanf (b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
	    &cpustats->user, &cpustats->nice, &cpustats->system,
	    &cpustats->idle, &cpustats->iowait, &cpustats->irq,
	    &cpustats->softirq, &cpustats->steal, &cpustats->guest,
	    &cpustats->guestn);
  else
    goto readerr;

  if ((b = strstr (buff, "ctxt ")))
    sscanf (b, "ctxt %Lu", &cpustats->nctxt);
  else
    goto readerr;

  /* Get the number of interrupts serviced since boot time, for each of the
   * possible system interrupts, including unnumbered architecture specific
   * interrupts  */
  if ((b = strstr (buff, "intr ")))
    sscanf (b, "intr %Lu ", &cpustats->nintr);
  else
    goto readerr;

  /* Not separated out until the 2.6.0-test4 */
  if ((b = strstr (buff, "softirq ")))
    sscanf (b, "softintr %Lu ", &cpustats->nsoftirq);

  return;

readerr:
  plugin_error (STATE_UNKNOWN, errno, "Error reading %s", PATH_PROC_STAT);
}
