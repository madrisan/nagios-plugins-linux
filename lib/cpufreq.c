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
#include <string.h>

#include "common.h"
#include "cpudesc.h"
#include "xasprintf.h"

#define PATH_SYS_SYSTEM		"/sys/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

#define SYSFS_PATH_MAX 255

enum cpufreq_sysfile_numeric_id
{
  CPUINFO_CUR_FREQ,
  CPUINFO_MIN_FREQ,
  CPUINFO_MAX_FREQ,
  CPUINFO_LATENCY,
  SCALING_CUR_FREQ,
  SCALING_MIN_FREQ,
  SCALING_MAX_FREQ,
  MAX_VALUE_FILES
};

static const char *cpufreq_sysfile_numeric[MAX_VALUE_FILES] = {
  [CPUINFO_CUR_FREQ] = "cpuinfo_cur_freq",
  [CPUINFO_MIN_FREQ] = "cpuinfo_min_freq",
  [CPUINFO_MAX_FREQ] = "cpuinfo_max_freq",
  [CPUINFO_LATENCY]  = "cpuinfo_transition_latency",
  [SCALING_CUR_FREQ] = "scaling_cur_freq",
  [SCALING_MIN_FREQ] = "scaling_min_freq",
  [SCALING_MAX_FREQ] = "scaling_max_freq"
};

enum cpufreq_sysfile_string_id
{
  SCALING_DRIVER,
  SCALING_GOVERNOR,
  SCALING_AVAILABLE_GOVERNORS,
  MAX_STRING_FILES
};

static const char *cpufreq_sysfile_string[MAX_STRING_FILES] = {
  [SCALING_DRIVER]   = "scaling_driver",
  [SCALING_GOVERNOR] = "scaling_governor",
  [SCALING_AVAILABLE_GOVERNORS] = "scaling_available_governors"
};

static char *
cpufreq_get_sysyfs_getline (const char *filename)
{
  FILE *fp;
  char *line;
  size_t len = 0;
  ssize_t chread;

  if ((fp = fopen (filename, "r")) == NULL)
    return NULL;
  
  if ((chread = getline (&line, &len, fp)) < 1)
    {
      fclose (fp);
      return NULL;
    }

  fclose (fp);
  return line;
}

static unsigned long
cpufreq_get_sysyfs_value (unsigned int cpunum,
			  enum cpufreq_sysfile_numeric_id which)
{
  char filename[SYSFS_PATH_MAX];
  char *line, *endptr;
  long value;

  if (which >= MAX_VALUE_FILES)
    return 0;

  snprintf (filename, SYSFS_PATH_MAX, PATH_SYS_CPU "/cpu%u/cpufreq/%s",
	    cpunum, cpufreq_sysfile_numeric[which]);

  if (NULL == (line = cpufreq_get_sysyfs_getline (filename)))
    return 0;

  errno = 0;
  value = strtoul (line, &endptr, 10);
  if ((endptr == line) || (errno == ERANGE))
    {
      free (line);
      return 0;
    }

  free (line);
  return value;
}

static char *
cpufreq_get_sysyfs_string (unsigned int cpunum,
			   enum cpufreq_sysfile_string_id which)
{
  char filename[SYSFS_PATH_MAX];
  char* line;
  size_t len = 0;

  if (which >= MAX_STRING_FILES)
    return NULL;

  snprintf (filename, SYSFS_PATH_MAX, PATH_SYS_CPU "/cpu%u/cpufreq/%s",
	    cpunum, cpufreq_sysfile_string[which]);
  
  if (NULL == (line = cpufreq_get_sysyfs_getline (filename)))
    return NULL;

  len = strlen (line);
  if (line[len-1] == '\n')
    line[len-1] = '\0';
  
  return line;
}

unsigned long
cpufreq_get_freq_kernel (unsigned int cpu)
{
  return  cpufreq_get_sysyfs_value (cpu, SCALING_CUR_FREQ);
}

int
cpufreq_get_hardware_limits (unsigned int cpu,
			     unsigned long *min, unsigned long *max)
{
  if ((!min) || (!max))
    return -EINVAL;

  *min = cpufreq_get_sysyfs_value (cpu, CPUINFO_MIN_FREQ);
  if (!*min)
    return -ENODEV;

  *max = cpufreq_get_sysyfs_value (cpu, CPUINFO_MAX_FREQ);
  if (!*max)
    return -ENODEV;

  return 0;
}

unsigned long
cpufreq_get_transition_latency (unsigned int cpu)
{
  return cpufreq_get_sysyfs_value (cpu, CPUINFO_LATENCY);
}

char *
cpufreq_get_driver (unsigned int cpu)
{
  return cpufreq_get_sysyfs_string (cpu, SCALING_DRIVER);
}

char *
cpufreq_get_governor (unsigned int cpu)
{
  return cpufreq_get_sysyfs_string (cpu, SCALING_GOVERNOR);
}

char *
cpufreq_get_available_governors (unsigned int cpu)
{
  return cpufreq_get_sysyfs_string (cpu, SCALING_AVAILABLE_GOVERNORS);
}

char *
cpufreq_freq_to_string (unsigned long freq)
{
  unsigned long tmp;

  if (freq > 1000000)
    { 
      tmp = freq % 10000;
      if (tmp >= 5000)
	freq += 10000;
      return xasprintf ("%u.%02u GHz", ((unsigned int) freq / 1000000),
		        ((unsigned int) (freq % 1000000) / 10000));
    }
  else if (freq > 100000)
    {
      tmp = freq % 1000;
      if (tmp >= 500)
	freq += 1000;
      return xasprintf ("%u MHz", ((unsigned int) freq / 1000));
    }
  else if (freq > 1000)
    {
      tmp = freq % 100;
      if (tmp >= 50)
	freq += 100;
      return xasprintf ("%u.%01u MHz", ((unsigned int) freq / 1000),
		        ((unsigned int) (freq % 1000) / 100));
    }
  else
    return xasprintf ("%lu kHz", freq);
}

char *
cpufreq_duration_to_string (unsigned long duration)
{
  unsigned long tmp;

  if (duration > 1000000)
    {
      tmp = duration % 10000;
      if (tmp >= 5000)
	duration += 10000;
      return xasprintf ("%u.%02u ms", ((unsigned int) duration / 1000000),
			((unsigned int) (duration % 1000000) / 10000));
    }
  else if (duration > 100000)
    {
      tmp = duration % 1000;
      if (tmp >= 500)
	duration += 1000;
      return xasprintf ("%u us", ((unsigned int) duration / 1000));
    }
  else if (duration > 1000)
    {
      tmp = duration % 100;
      if (tmp >= 50)
        duration += 100;
      return xasprintf ("%u.%01u us", ((unsigned int) duration / 1000),
			((unsigned int) (duration % 1000) / 100));
    }
  else
    return xasprintf ("%lu ns", duration);
}
