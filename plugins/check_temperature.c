// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014-2021,2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that monitors the hardware's temperature.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Based on the code of the acpiclient tool:
 *  <http://sourceforge.net/p/acpiclient/code/ci/master/tree/acpi.c>
 *
 * See also the official kernel documentation:
 *  <https://www.kernel.org/doc/Documentation/thermal/sysfs-api.txt>
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "sysfsparser.h"
#include "thresholds.h"
#include "xalloc.h"

enum
{
  TEMP_KELVIN,
  TEMP_CELSIUS,
  TEMP_FAHRENHEIT
};

static const char *program_copyright =
  "Copyright (C) 2014-2021,2022 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "fahrenheit", required_argument, NULL, 'f'},
  {(char *) "kelvin", required_argument, NULL, 'k'},
  {(char *) "list", no_argument, NULL, 'l'},
  {(char *) "thermal_zone", required_argument, NULL, 't'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin monitors the hardware's temperature.\n", out);
  fprintf (out, "It requires the sysfs tree %s to be mounted and readable.\n",
           sysfsparser_thermal_sysfs_path ());
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-f|-k] [-t <thermal_zone_num>] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -f, --fahrenheit  use fahrenheit as the temperature unit\n", out);
  fputs ("  -k, --kelvin      use kelvin as the temperature unit\n", out);
  fputs ("  -l, --list        list all the thermal sensors reported by the"
	 " kernel\n", out);
  fputs ("  -t, --thermal_zone    only consider a specific thermal zone\n", out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_NOTE, out);
  fputs ("  If the option '-t|--thermal_zone' is not specified, then the"
	 " thermal zone\n"
	 "  with the highest temperature is selected. Note that this zone"
	 " may change\n"
	 "  at each plugin execution.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --list\n", program_name);
  fprintf (out, "  %s -w 80 -c 90\n", program_name);
  fprintf (out, "  %s -t 0 -w 80 -c 90\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static _Noreturn void
print_version (void)
{
  printf ("%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs (program_copyright, stdout);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
}

static double
get_real_temp (unsigned long temperature, char **scale, int temp_units)
{
  double real_temp = (double) temperature / 1000.0;
  const double absolute_zero = 273.1;

  switch (temp_units)
    {
    case TEMP_CELSIUS:
      *scale = "°C";
      break;
    case TEMP_FAHRENHEIT:
      real_temp = (real_temp * 1.8) + 32;
      *scale = "°F";
      break;
    case TEMP_KELVIN:
    default:
      real_temp += absolute_zero;
      *scale = "°K";
      break;
    }

  return (real_temp);
}

#ifndef NPL_TESTING
int
main (int argc, char **argv)
{
  int c, temperature_unit = TEMP_CELSIUS;
  unsigned int thermal_zone, selected_thermal_zone = ALL_THERMAL_ZONES;
  char *critical = NULL, *warning = NULL,
       *end, *type, *scale;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "fklt:c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'f':
	  temperature_unit = TEMP_FAHRENHEIT;
	  break;
	case 'k':
	  temperature_unit = TEMP_KELVIN;
	  break;
	case 'l':
	  sysfsparser_thermal_listall ();
	  return STATE_UNKNOWN;
	case 't':
	  errno = 0;
	  selected_thermal_zone = strtoul (optarg, &end, 10);
	  if (errno != 0 || optarg == end || (end != NULL && *end != '\0'))
	    plugin_error (STATE_UNKNOWN, 0,
			  "the option '-t' requires an integer");
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

        }
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  sysfsparser_check_for_sysfs ();

  int max_temp =
    sysfsparser_thermal_get_temperature (selected_thermal_zone,
					 &thermal_zone, &type);
  double real_temp = get_real_temp (max_temp, &scale, temperature_unit);

  status = get_status (real_temp, my_threshold);
  free (my_threshold);

  /* check for the related critical temperature, if any */
  int crit_temp =
    sysfsparser_thermal_get_critical_temperature (thermal_zone) / 1000;

  printf ("%s %s - +%.1f%s (thermal zone: %u [%s], type: \"%s\") | temp=%u%c",
	  program_name_short, state_text (status), real_temp, scale,
	  thermal_zone, sysfsparser_thermal_get_device (thermal_zone),
	  type ? type : "n/a", (unsigned int) real_temp,
	  (temperature_unit == TEMP_KELVIN) ? 'K' :
	  (temperature_unit == TEMP_FAHRENHEIT) ? 'F' : 'C');

  if (crit_temp > 0 && ALL_THERMAL_ZONES != selected_thermal_zone)
    printf (";0;%d", crit_temp);

  putchar ('\n');

  return status;
}
#endif			/* NPL_TESTING */
