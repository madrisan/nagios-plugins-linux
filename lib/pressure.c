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
#include "string-macros.h"
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

  dbg ("reading file %s\n", procpath);
  while ((chread = getline (&line, &len, fp)) != -1)
    {
      dbg ("line: %s", line);
      label_len = strlen (label);
      if (STREQLEN (line, label, label_len))
	{
	  dbg (" \\ matching label \"%s\"\n", label);
	  rc =
	    sscanf (line + label_len + 1,
		    "avg10=%32lf avg60=%32lf avg300=%32lf total=%llu",
		    &psi_stat->avg10,
		    &psi_stat->avg60,
		    &psi_stat->avg300,
		    &psi_stat->total);
	  dbg (" \\ avg10=%g avg60=%g avg300=%g total=%llu\n",
	       psi_stat->avg10, psi_stat->avg60, psi_stat->avg300,
	       psi_stat->total);
	  break;
	}
    }

  free (line);
  fclose (fp);

  if (rc < 4)
    plugin_error (STATE_UNKNOWN, errno, "error reading %s", procpath);

  return 0;
}

int
proc_psi_read_cpu (struct proc_psi_oneline **psi_cpu,
		   unsigned long long *starvation)
{
  struct proc_psi_oneline psi, *stats = *psi_cpu;
  unsigned long long total;

  if (NULL == stats)
    {
      stats = xmalloc (sizeof (struct proc_psi_oneline));
      *psi_cpu = stats;
    }

  proc_psi_parser (&psi, PATH_PSI_PROC_CPU, "some");
  stats->avg10 = psi.avg10;
  stats->avg60 = psi.avg60;
  stats->avg300 = psi.avg300;
  total = psi.total;

  /* calculate the starvation (in microseconds) per second */
  sleep (1);

  proc_psi_parser (&psi, PATH_PSI_PROC_CPU, "some");
  *starvation = psi.total - total;
  dbg ("delta (over 1sec): %llu (%llu - %llu)\n",
       *starvation, psi.total, total);

  return 0;
}

static int
proc_psi_read (struct proc_psi_twolines **psi_io,
	       unsigned long long *starvation, char *procfile)
{
  struct proc_psi_oneline psi;
  struct proc_psi_twolines *stats = *psi_io;
  unsigned long long total;

  if (NULL == stats)
    {
      stats = xmalloc (sizeof (struct proc_psi_twolines));
      *psi_io = stats;
    }

  proc_psi_parser (&psi, procfile, "some");
  stats->some_avg10 = psi.avg10;
  stats->some_avg60 = psi.avg60;
  stats->some_avg300 = psi.avg300;
  total = psi.total;

  sleep (1);

  proc_psi_parser (&psi, procfile, "some");
  *starvation = psi.total - total;
  dbg ("delta (over 1sec) form some: %llu (%llu - %llu)\n",
       *starvation, psi.total, total);

  proc_psi_parser (&psi, procfile, "full");
  stats->full_avg10 = psi.avg10;
  stats->full_avg60 = psi.avg60;
  stats->full_avg300 = psi.avg300;
  total = psi.total;

  sleep (1);

  proc_psi_parser (&psi, procfile, "full");
  *(starvation + 1) = psi.total - total;
  dbg ("delta (over 1sec) for full: %llu (%llu - %llu)\n",
       *(starvation + 1), psi.total, total);

  return 0;
}

int
proc_psi_read_io (struct proc_psi_twolines **psi_io,
		  unsigned long long *starvation)
{
  return proc_psi_read (psi_io, starvation, PATH_PSI_PROC_IO);
}

int
proc_psi_read_memory (struct proc_psi_twolines **psi_memory,
		      unsigned long long *starvation)
{
  return proc_psi_read (psi_memory, starvation, PATH_PSI_PROC_MEMORY);
}
