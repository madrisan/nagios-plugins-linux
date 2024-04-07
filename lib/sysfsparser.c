// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014-2024 Davide Madrisan <davide.madrisan@gmail.com>
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE	/* activate extra prototypes for glibc */
#endif

#include <linux/limits.h>
#include <linux/magic.h>

#include <sys/types.h>
#include <sys/vfs.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "sysfsparser.h"
#include "xasprintf.h"

#define PATH_SYS_SYSTEM		PATH_SYS "/devices/system"
#define PATH_SYS_CPU		PATH_SYS_SYSTEM "/cpu"

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

void
sysfsparser_check_for_sysfs (void)
{
  struct statfs statfsbuf;
  char sysfs_mount[NAME_MAX];

  snprintf (sysfs_mount, NAME_MAX, "%s", PATH_SYS);
  if (statfs (sysfs_mount, &statfsbuf) < 0
      || statfsbuf.f_type != SYSFS_MAGIC)
    plugin_error (STATE_UNKNOWN, 0,
		  "The sysfs filesystem (" PATH_SYS ") is not mounted");
}

bool
sysfsparser_path_exist (const char *path, ...)
{
  char *filename;
  va_list args;

  va_start (args, path);
  if (vasprintf (&filename, path, args) < 0)
    plugin_error (STATE_UNKNOWN, errno, "vasprintf has failed");
  va_end (args);

  return access (filename, F_OK) == 0;
}

void
sysfsparser_opendir(DIR **dirp, const char *path, ...)
{
  char *dirname;
  va_list args;

  va_start (args, path);
  if (vasprintf (&dirname, path, args) < 0)
    plugin_error (STATE_UNKNOWN, errno, "vasprintf has failed");
  va_end (args);

  if ((*dirp = opendir (dirname)) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", dirname);
}

void sysfsparser_closedir(DIR *dirp)
{
  closedir(dirp);
}

struct dirent *
sysfsparser_readfilename(DIR *dirp, unsigned int flags)
{
  for (;;)
    {
      struct dirent *dp;
      errno = 0;
      if ((dp = readdir (dirp)) == NULL)
	{
	  if (errno != 0)
	    plugin_error (STATE_UNKNOWN, errno, "readdir() failure");
	  else
	    return NULL;		/* end-of-directory */
	}

      /* ignore directory entries */
      if (STREQ (dp->d_name, ".") || STREQ (dp->d_name, ".."))
	continue;

      if (dp->d_type & flags)
	return dp;
    }
}

char *
sysfsparser_getline (const char *format, ...)
{
  FILE *fp;
  char *filename, *line = NULL;
  size_t len = 0;
  ssize_t chread;
  va_list args;

  va_start (args, format);
  if (vasprintf (&filename, format, args) < 0)
    plugin_error (STATE_UNKNOWN, errno, "vasprintf has failed");
  va_end (args);

  dbg ("reading sysfs data: %s\n", filename);
  if ((fp = fopen (filename, "r")) == NULL)
    {
      dbg ("  \\ error: %s\n", strerror (errno));
      return NULL;
    }

  chread = getline (&line, &len, fp);
  fclose (fp);

  if (chread < 1)
    return NULL;

  len = strlen (line);
  if (line[len-1] == '\n')
    line[len-1] = '\0';

  dbg ("  \\ \"%s\"\n", line);
  return line;
}

int
sysfsparser_getvalue (unsigned long long *value, const char *format, ...)
{
  char *line, *endptr, *filename;
  int retvalue = 0;
  va_list args;

  va_start (args, format);
  if (vasprintf (&filename, format, args) < 0)
    plugin_error (STATE_UNKNOWN, errno, "vasprintf has failed");
  va_end (args);

  if (NULL == (line = sysfsparser_getline ("%s", filename)))
    return -1;

  errno = 0;
  *value = strtoull (line, &endptr, 0);
  if ((endptr == line) || (errno == ERANGE))
    retvalue = -1;

  free (line);
  return retvalue;
}

int
sysfsparser_linelookup_numeric (char *line, char *pattern, long long *value)
{
  char *p;
  size_t len = strlen (pattern);

  if (!*line || (len + 1 > strlen (line)))
    return 0;

  p = line + len;
  if (strncmp (line, pattern, len) || !isspace (*p))
    return 0;

  *value = strtoll (p, NULL, 10);
  return 1;
}

static unsigned long
sysfsparser_cpufreq_get_value (unsigned int cpunum,
			       enum sysfsparser_cpufreq_sysfile_numeric_id which)
{
  char *line, *endptr;
  long value;

  if (which >= MAX_VALUE_FILES)
    return 0;

  line =
    sysfsparser_getline (PATH_SYS_CPU "/cpu%u/cpufreq/%s", cpunum,
			 sysfsparser_devices_system_cpu_file_numeric[which]);
  if (NULL == line)
    return 0;

  errno = 0;
  value = strtoul (line, &endptr, 10);
  if ((endptr == line) || (errno == ERANGE))
    value = 0;

  free (line);
  return value;
}

static char *
sysfsparser_cpufreq_get_string (unsigned int cpunum,
				enum sysfsparser_cpufreq_sysfile_string_id which)
{
  char *line;
  size_t len = 0;

  if (which >= MAX_STRING_FILES)
    return NULL;

  line =
    sysfsparser_getline (PATH_SYS_CPU "/cpu%u/cpufreq/%s", cpunum,
			 sysfsparser_devices_system_cpu_file_string[which]);
  if (NULL == line)
    return 0;

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

/* Thermal Sensors function  */

#define PATH_SYS_ACPI   PATH_SYS "/class"
#define PATH_SYS_ACPI_THERMAL   PATH_SYS_ACPI "/thermal"

/* Thermal zone device sys I/F, created once it's registered:
 * /sys/class/thermal/thermal_zone[0-*]:
 *    |---type:                   Type of the thermal zone
 *    |---temp:                   Current temperature
 *    |---mode:                   Working mode of the thermal zone
 *    |---policy:                 Thermal governor used for this zone
 *    |---trip_point_[0-*]_temp:  Trip point temperature
 *    |---trip_point_[0-*]_type:  Trip point type
 *    |---trip_point_[0-*]_hyst:  Hysteresis value for this trip point
 *    |---emul_temp:              Emulated temperature set node
 */

bool
sysfsparser_thermal_kernel_support ()
{
  if (chdir (PATH_SYS_ACPI_THERMAL) < 0)
    return false;

  return true;
}

const char *
sysfsparser_thermal_sysfs_path ()
{
  return PATH_SYS_ACPI_THERMAL;
}

int
sysfsparser_thermal_get_critical_temperature (unsigned int thermal_zone)
{
  int i, err;
  unsigned long long crit_temp = 0;

  /* as far as I can see, the only possible trip points are:
   *  'critical', 'passive', 'active0', and 'active1'
   * Four (optional) entries max.   */
  for (i = 0; i < 4; i++)
    {
      char *type;
      type = sysfsparser_getline (PATH_SYS_ACPI_THERMAL
				  "/thermal_zone%u/trip_point_%d_type",
				  thermal_zone, i);
      if (NULL == type)
	continue;

      /* cat /sys/class/thermal/thermal_zone0/trip_point_0_temp
       *  98000
       * cat /sys/class/thermal/thermal_zone0/trip_point_0_type
       *  critical   */
      if (!STRPREFIX (type, "critical"))
	continue;

      err = sysfsparser_getvalue (&crit_temp, PATH_SYS_ACPI_THERMAL
				  "/thermal_zone%u/trip_point_%d_temp",
				  thermal_zone, i);
      if (err < 0)
	plugin_error (STATE_UNKNOWN, 0,
		      "an error has occurred while reading "
		      PATH_SYS_ACPI_THERMAL
		      "/thermal_zone%u/trip_point_%d_temp", thermal_zone, i);

      if (crit_temp > 0)
	dbg ("a critical trip point has been found: %.2f degrees C\n",
	     (float) (crit_temp / 1000.0));

      free (type);
      break;
    }

  return crit_temp;
}

const char *
sysfsparser_thermal_get_device (unsigned int thermal_zone)
{
  char *device;
  device = sysfsparser_getline (PATH_SYS_ACPI_THERMAL
				"/thermal_zone%u/device/path",
				thermal_zone);
  if (NULL == device)
    return "Virtual device";

  return STRTRIMPREFIX (device, "\\_TZ_.");
}

int
sysfsparser_thermal_get_temperature (unsigned int selected_zone,
				     unsigned int *zone, char **type)
{
  DIR *d;
  struct dirent *de;
  bool found_data = false;
  unsigned int thermal_zone;
  unsigned long long max_temp = 0, temp = 0;

  if (!sysfsparser_thermal_kernel_support ())
    plugin_error (STATE_UNKNOWN, 0, "no ACPI thermal support in kernel "
		  "or incorrect path (\"%s\")", PATH_SYS_ACPI_THERMAL);

  if ((d = opendir (".")) == NULL)
    plugin_error (STATE_UNKNOWN, errno,
		  "cannot open() " PATH_SYS_ACPI_THERMAL);

  while ((de = readdir (d)))
    {
      /* ignore directory entries */
      if (STREQ (de->d_name, ".") || STREQ (de->d_name, ".."))
	continue;
      /* ignore all files but 'thermal_zone[0-*]' ones */
      if (!STRPREFIX (de->d_name, "thermal_zone"))
	continue;

      thermal_zone = strtoul (de->d_name + 12, NULL, 10);
      if ((selected_zone != ALL_THERMAL_ZONES) &&
	  (selected_zone != thermal_zone))
	continue;

      /* temperatures are stored in the files
       *  /sys/class/thermal/thermal_zone[0-9]/temp	  */
      sysfsparser_getvalue (&temp, PATH_SYS_ACPI_THERMAL "/%s/temp",
				   de->d_name);
      *type = sysfsparser_getline (PATH_SYS_ACPI_THERMAL "/%s/type",
				   de->d_name);

      /* If a thermal zone is not asked by user, get the highest temperature
       * reported by sysfs */
      dbg ("thermal information found: %.2f°C, zone: %u, type: %s\n",
	   (float) (temp / 1000.0), thermal_zone, *type);
      found_data = true;
      if (max_temp < temp || 0 == max_temp)
	{
	  max_temp = temp;
	  *zone = thermal_zone;
	}
    }
  closedir (d);

  if (false == found_data)
    {
      if (selected_zone == ALL_THERMAL_ZONES)
	plugin_error (STATE_UNKNOWN, 0,
		      "no thermal information has been found");
      else
	plugin_error (STATE_UNKNOWN, 0,
		      "no thermal information for zone '%u'", selected_zone);
    }

  return max_temp;
}

void
sysfsparser_thermal_listall ()
{
  struct dirent **namelist;
  unsigned int thermal_zone;
  char *type;
  int i, n, crit_temp;

  if (!sysfsparser_thermal_kernel_support ())
    plugin_error (STATE_UNKNOWN, errno,
		  "no ACPI thermal support in kernel "
		  "or incorrect path (\"%s\")", PATH_SYS_ACPI_THERMAL);

  n = scandir (PATH_SYS_ACPI_THERMAL, &namelist, 0, alphasort);
  if (-1 == n)
    plugin_error (STATE_UNKNOWN, errno,
		  "cannot scandir() " PATH_SYS_ACPI_THERMAL);

  printf ("Thermal zones reported by the linux kernel (%s):\n",
	  sysfsparser_thermal_sysfs_path ());

  for (i = 0; i < n; i++)
    {
      if (STRPREFIX (namelist[i]->d_name, "thermal_zone"))
	{
	  thermal_zone = strtoul (namelist[i]->d_name + 12, NULL, 10);
	  type = sysfsparser_getline (PATH_SYS_ACPI_THERMAL "/%s/type",
				      namelist[i]->d_name);
	  crit_temp =
	    sysfsparser_thermal_get_critical_temperature (thermal_zone);

	  fprintf (stdout, " - zone %2u [%s], type \"%s\""
		   , thermal_zone
		   , sysfsparser_thermal_get_device (thermal_zone)
		   , type ? type : "n/a");
	  if (crit_temp > 0)
	    fprintf (stdout, ", critical trip point at %d°C", crit_temp / 1000);
	  fputs ("\n", stdout);
	}

      free (namelist[i]);
    }

  free(namelist);
}
