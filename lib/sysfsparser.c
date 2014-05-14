/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for parsing the sysfs filesystem
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sysfsparser.h"
#include "xasprintf.h"

#define PATH_SYS_SYSTEM		"/sys/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

#define SYSFS_PATH_MAX 255

enum sysfsparser_cpufreq_sysfile_numeric_id
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

static const char *
  sysfsparser_devices_system_cpu_file_numeric[MAX_VALUE_FILES] = {
  [CPUINFO_CUR_FREQ] = "cpuinfo_cur_freq",
  [CPUINFO_MIN_FREQ] = "cpuinfo_min_freq",
  [CPUINFO_MAX_FREQ] = "cpuinfo_max_freq",
  [CPUINFO_LATENCY]  = "cpuinfo_transition_latency",
  [SCALING_CUR_FREQ] = "scaling_cur_freq",
  [SCALING_MIN_FREQ] = "scaling_min_freq",
  [SCALING_MAX_FREQ] = "scaling_max_freq"
};

enum sysfsparser_cpufreq_sysfile_string_id
{
  SCALING_DRIVER,
  SCALING_GOVERNOR,
  SCALING_AVAILABLE_GOVERNORS,
  SCALING_AVAILABLE_FREQS,
  MAX_STRING_FILES
};

static const char *
  sysfsparser_devices_system_cpu_file_string[MAX_STRING_FILES] = {
  [SCALING_DRIVER]   = "scaling_driver",
  [SCALING_GOVERNOR] = "scaling_governor",
  [SCALING_AVAILABLE_GOVERNORS] = "scaling_available_governors",
  [SCALING_AVAILABLE_FREQS] = "scaling_available_frequencies"
};

char *
sysfsparser_getline (const char *filename)
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

unsigned long
sysfsparser_getvalue (const char *filename)
{
  char *line, *endptr;
  unsigned long value;

  if (NULL == (line = sysfsparser_getline (filename)))
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

static unsigned long
sysfsparser_cpufreq_get_value (unsigned int cpunum,
			       enum sysfsparser_cpufreq_sysfile_numeric_id which)
{
  char filename[SYSFS_PATH_MAX];
  char *line, *endptr;
  long value;

  if (which >= MAX_VALUE_FILES)
    return 0;

  snprintf (filename, SYSFS_PATH_MAX, PATH_SYS_CPU "/cpu%u/cpufreq/%s",
	    cpunum, sysfsparser_devices_system_cpu_file_numeric[which]);

  if (NULL == (line = sysfsparser_getline (filename)))
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
sysfsparser_cpufreq_get_string (unsigned int cpunum,
				enum sysfsparser_cpufreq_sysfile_string_id which)
{
  char filename[SYSFS_PATH_MAX];
  char* line;
  size_t len = 0;

  if (which >= MAX_STRING_FILES)
    return NULL;

  snprintf (filename, SYSFS_PATH_MAX, PATH_SYS_CPU "/cpu%u/cpufreq/%s",
	    cpunum, sysfsparser_devices_system_cpu_file_string[which]);
  
  if (NULL == (line = sysfsparser_getline (filename)))
    return NULL;

  len = strlen (line);
  if (line[len-1] == '\n')
    line[len-1] = '\0';
  
  return line;
}

/* CPU Freq functions  */

int
sysfsparser_cpufreq_get_hardware_limits (unsigned int cpu,
					 unsigned long *min,
					 unsigned long *max)
{
  if ((!min) || (!max))
    return -EINVAL;

  *min = sysfsparser_cpufreq_get_value (cpu, CPUINFO_MIN_FREQ);
  if (!*min)
    return -ENODEV;

  *max = sysfsparser_cpufreq_get_value (cpu, CPUINFO_MAX_FREQ);
  if (!*max)
    return -ENODEV;

  return 0;
}

unsigned long
sysfsparser_cpufreq_get_freq_kernel (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_value (cpu, SCALING_CUR_FREQ);
}

char *
sysfsparser_cpufreq_get_available_freqs (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_string (cpu, SCALING_AVAILABLE_FREQS);
}

unsigned long
sysfsparser_cpufreq_get_transition_latency (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_value (cpu, CPUINFO_LATENCY);
}

char *
sysfsparser_cpufreq_get_driver (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_string (cpu, SCALING_DRIVER);
}

char *
sysfsparser_cpufreq_get_governor (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_string (cpu, SCALING_GOVERNOR);
}

char *
sysfsparser_cpufreq_get_available_governors (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_string (cpu, SCALING_AVAILABLE_GOVERNORS);
}
