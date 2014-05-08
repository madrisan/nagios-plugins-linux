/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the CPU frequency configuration
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "cpudesc.h"

#define PATH_SYS_SYSTEM		"/sys/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

#define SYSFS_PATH_MAX 255

enum cpufreq_sysfile_id
{
  CPUINFO_MIN_FREQ,
  CPUINFO_MAX_FREQ,
  MAX_VALUE_FILES
};

static const char *cpufreq_sysfile[MAX_VALUE_FILES] = {
  [CPUINFO_MIN_FREQ] = "cpuinfo_min_freq",
  [CPUINFO_MAX_FREQ] = "cpuinfo_max_freq",
};

static long
cpufreq_get_freq (unsigned int cpunum, enum cpufreq_sysfile_id which)
{
  char filename[SYSFS_PATH_MAX];
  char *line, *endptr;
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  long value;

  if (which >= MAX_VALUE_FILES )
    return 0;

  snprintf (filename, SYSFS_PATH_MAX, PATH_SYS_CPU "/cpu%u/cpufreq/%s",
            cpunum, cpufreq_sysfile[which]);

  if ((fp = fopen (filename,  "r")) == NULL)
    return 0;

  if ((chread = getline (&line, &len, fp)) < 1)
    {
      fclose (fp);
      return 0;
    }

  fclose (fp);

  errno = 0;
  value = strtol (line, &endptr, 10);
  if ((errno == ERANGE && (value == LONG_MAX || value == LONG_MIN))
             || (errno != 0 && value == 0))
    goto err;

  if (endptr == line)
    goto err;	/* No digits were found */

  free (line);
  return value;

err:
  free (line);
  return 0;
}

long
cpufreq_get_freq_min (void)
{
   int i;
   long cpufreq_min = 0;

   for (i = 0; i < get_processor_number_total (); i++)
     {
       long curr = cpufreq_get_freq (i, CPUINFO_MIN_FREQ);
       if (curr > 0 && ((cpufreq_min > curr) || cpufreq_min == 0))
	cpufreq_min = curr;
     }

   return (cpufreq_min / 1000);
}

long
cpufreq_get_freq_max (void)
{
  int i;
  long cpufreq_max = 0;

  for (i = 0; i < get_processor_number_total (); i++)
     {
       long curr = cpufreq_get_freq (i, CPUINFO_MAX_FREQ);
       if (curr > cpufreq_max)
	cpufreq_max = curr;
     }

   return (cpufreq_max / 1000);
}
