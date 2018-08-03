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
#include "getenv.h"
#include "logging.h"
#include "messages.h"
#include "procparser.h"
#include "system.h"
#include "xalloc.h"
#include "xasprintf.h"

const char *
get_path_proc_stat ()
{
  const char *env_procstat = secure_getenv ("NPL_TEST_PATH_PROCSTAT");
  if (env_procstat)
    return env_procstat;

  return "/proc/stat";
}

/* Fill the cpu_stats structure pointed with the values found in the
 * proc filesystem */

void
cpu_stats_get_time (struct cpu_time * __restrict cputime, unsigned int lines)
{
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  char *line = NULL;
  bool found;
  const char *procpath = get_path_proc_stat ();

  if ((fp = fopen (procpath, "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", procpath);

  memset (cputime, '\0', lines * sizeof (struct cpu_time));

  found = false;
  while ((chread = getline (&line, &len, fp)) != -1)
    {
      if (!strncmp (line, "cpu ", 4))
	{
	  cputime->cpuname = xstrdup ("cpu");
	  sscanf (line,
		  "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
		  &cputime->user, &cputime->nice, &cputime->system,
		  &cputime->idle, &cputime->iowait, &cputime->irq,
		  &cputime->softirq, &cputime->steal, &cputime->guest,
		  &cputime->guestn);
	  dbg ("line: %s", line);
	  dbg (" \\ %s >> [1]user:%llu ... [5]iowait:%llu ...\n",
	       cputime->cpuname, cputime->user, cputime->iowait);
	  found = true;
	  if (lines == 1)
	    break;
	}
      else if (!strncmp (line, "cpu", 3))
	{
	  char *endptr;
	  unsigned int cpunum = strtol (line + 3, &endptr, 10);
	  if (lines <= cpunum + 1)
	    plugin_error (STATE_UNKNOWN, 0,
			  "BUG: %s(): lines(%u) <= cpunum(%u) + 1",
			  __FUNCTION__, lines, cpunum);

	  unsigned int i = cpunum + 1;
	  cputime[i].cpuname = xasprintf ("cpu%u", cpunum);
	  sscanf (endptr,
		  "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
		  &cputime[i].user, &cputime[i].nice,
		  &cputime[i].system, &cputime[i].idle,
		  &cputime[i].iowait, &cputime[i].irq,
		  &cputime[i].softirq, &cputime[i].steal,
		  &cputime[i].guest, &cputime[i].guestn);
	  dbg ("line: %s", line);
	  dbg (" \\ %s >> [1]user:%llu ... [5]iowait:%llu ...\n",
	       cputime[i].cpuname, cputime[i].user, cputime[i].iowait);
	}
    }

  free (line);
  fclose (fp);

  if (!found)
    plugin_error (STATE_UNKNOWN, 0,
		  "%s: pattern not found: 'cpu '", procpath);
}

static unsigned long long
cpu_stats_get_value_with_pattern (const char *pattern, bool mandatory)
{
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  char *line = NULL;
  bool found;
  unsigned long long value;
  const char *procpath = get_path_proc_stat ();

  if ((fp = fopen (procpath, "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", procpath);

  value = 0;
  found = false;

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      if (!strncmp (line, pattern, strlen (pattern)))
	{
	  sscanf (line + strlen (pattern), "%llu", &value);
	  dbg ("line: %s \\ value for '%s': %llu\n", line, pattern, value);
	  found = true;
	  break;
	}
    }

  free (line);
  fclose (fp);

  if (!found && mandatory)
    plugin_error (STATE_UNKNOWN, 0,
		  "%s: pattern not found: '%s'", procpath, pattern);

  return value;
}

/* wrappers */

unsigned long long
cpu_stats_get_cswch ()
{
  return cpu_stats_get_value_with_pattern ("ctxt ", true);
}

unsigned long long
cpu_stats_get_intr ()
{
  return cpu_stats_get_value_with_pattern ("intr ", true);
}

unsigned long long
cpu_stats_get_softirq ()
{
  /* Not separated out until the 2.6.0-test4, hence 'false' */
  return cpu_stats_get_value_with_pattern ("softirq ", false);
}
