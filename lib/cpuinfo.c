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
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpuinfo.h"
#include "messages.h"

#define BUFFSIZE 0x1000
static char buff[BUFFSIZE];

#define PROC_STAT   "/proc/stat"

/* Fill the proc_cpu structure pointed with the values found in the 
 * proc filesystem */

void
proc_cpu_read (struct proc_cpu *cpuinfo)
{
  static int fd;
  const char *b;

  if (fd)
    lseek (fd, 0L, SEEK_SET);
  else
    {
      fd = open (PROC_STAT, O_RDONLY, 0);
      if (fd == -1)
	plugin_error (STATE_UNKNOWN, errno, "Error opening %s", PROC_STAT);
    }

  read (fd, buff, BUFFSIZE - 1);

  cpuinfo->iowait = 0;	/* not separated out until the 2.5.41 kernel */
  cpuinfo->irq = 0;	/* not separated out until the 2.6.0-test4 */
  cpuinfo->softirq = 0;	/* not separated out until the 2.6.0-test4 */
  cpuinfo->steal = 0;	/* not separated out until the 2.6.11 */
  cpuinfo->guest = 0;	/* since Linux 2.6.24 */
  cpuinfo->guestn = 0;	/* since Linux 2.6.33 */

  b = strstr (buff, "cpu ");
  if (b)
    sscanf (b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", &cpuinfo->user,
	    &cpuinfo->nice, &cpuinfo->system, &cpuinfo->idle,
	    &cpuinfo->iowait, &cpuinfo->irq, &cpuinfo->softirq,
	    &cpuinfo->steal, &cpuinfo->guest, &cpuinfo->guestn);
  else
    plugin_error (STATE_UNKNOWN, errno, "Error reading %s", PROC_STAT);
}
