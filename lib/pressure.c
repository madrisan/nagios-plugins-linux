// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking 
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "messages.h"
#include "pressure.h"
#include "xalloc.h"

static int
proc_psi_parser (struct proc_psi_oneline *psi_stat,
		 const char *procpath, char *label)
{
  FILE *fp;
  int rc = 0;
  size_t len = 0, label_len;
  ssize_t chread;
  char *line = NULL;

  if ((fp = fopen (procpath, "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", procpath);

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      label_len = strlen (label);
      if (!strncmp (line, label, label_len))
	{
	  rc =
	    sscanf (line + label_len + 1,
		    "avg10=%32lf avg60=%32lf avg300=%32lf total=%llu",
		    &psi_stat->avg10,
		    &psi_stat->avg60,
		    &psi_stat->avg300,
		    &psi_stat->total);
	  dbg ("line: %s", line);
          dbg (" \\ %s >> avg10=%g avg60=%g avg300=%g total=%llu\n",
	       label, psi_stat->avg10, psi_stat->avg60, psi_stat->avg300,
	       psi_stat->total);
	}
    }

  free (line);
  fclose (fp);

  if (rc < 4)
    plugin_error (STATE_UNKNOWN, errno, "error reading %s", procpath);

  return 0;
}

int
proc_psi_read_cpu (struct proc_psi_oneline **psi_cpu)
{
  struct proc_psi_oneline psi, *stats = *psi_cpu;

  proc_psi_parser (&psi, PATH_PSI_PROC_CPU, "some");

  if (NULL == stats)
    {
      stats = xmalloc (sizeof (struct proc_psi_oneline));
      *psi_cpu = stats;
    }

  stats->avg10 = psi.avg10;
  stats->avg60 = psi.avg60;
  stats->avg300 = psi.avg300;
  stats->total = psi.total;

  sleep (1);
  proc_psi_parser (&psi, PATH_PSI_PROC_CPU, "some");
  stats->starved_nsecs_per_second = psi.total - stats->total;

  return 0;
}

int
proc_psi_read_io (struct proc_psi_twolines **psi_io)
{
  struct proc_psi_oneline psi;

  if (!proc_psi_parser (&psi, PATH_PSI_PROC_IO, "some"))
    return -1;

  if (!proc_psi_parser (&psi, PATH_PSI_PROC_IO, "full"))
    return -1;

  return 0;
}

int
proc_psi_read_memory (struct proc_psi_twolines **psi_memory)
{
  struct proc_psi_oneline psi;

  if (!proc_psi_parser (&psi, PATH_PSI_PROC_MEMORY, "some"))
    return -1;

  if (!proc_psi_parser (&psi, PATH_PSI_PROC_MEMORY, "full"))
    return -1;

  return 0;
}
