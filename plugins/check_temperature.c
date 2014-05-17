/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that monitors the hardware's temperature
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

#include <sys/types.h>
#include <dirent.h>
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

static bool verbose = false;

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "fahrenheit", required_argument, NULL, 'f'},
  {(char *) "kelvin", required_argument, NULL, 'k'},
  {(char *) "thermal_zone", required_argument, NULL, 't'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "verbose", no_argument, NULL, 'v'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin monitors the hardware's temperature.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-f|-k] [-t <thermal_zone_num>] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -f, --fahrenheit  use fahrenheit as the temperature unit\n", out);
  fputs ("  -k, --kelvin      use kelvin as the temperature unit\n", out);
  fputs ("  -t, --thermal_zone    only consider a specific thermal zone\n", out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
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
      *scale = "degrees C";
      break;
    case TEMP_FAHRENHEIT:
      real_temp = (real_temp * 1.8) + 32;
      *scale = "degrees F";
      break;
    case TEMP_KELVIN:
    default:
      real_temp += absolute_zero;
      *scale = "kelvin";
      break;
    }

  return (real_temp);
}

int 
get_critical_trip_point (int thermal_zone)
{
  char *type;
  int i, crit_temp = 0;

  for (i = 0; i < 5; i++)
    {
      type = sysfsparser_getline (PATH_SYS_ACPI_THERMAL
				  "/thermal_zone%d/trip_point_%d_type",
				  thermal_zone, i);
      if (NULL == type)
	continue;
      if (strncmp (type, "critical", strlen ("critical")))
	continue;

      crit_temp = sysfsparser_getvalue (PATH_SYS_ACPI_THERMAL
					"/thermal_zone%d/trip_point_%d_temp",
					thermal_zone, i);

      if (verbose && (crit_temp > 0))
	printf ("found a critical trip point: %d (%dC)\n",
		crit_temp, crit_temp / 1000);

      free (type);
      break;
    }

  return crit_temp / 1000;
}

int
main (int argc, char **argv)
{
  int c;
  char *critical = NULL, *warning = NULL, *end;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  DIR *d;
  struct dirent *de;
  bool found_data = false;
  int selected_thermal_zone = -1;
  int temperature_unit = TEMP_CELSIUS;

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
	case 't':
	  errno = 0;
	  selected_thermal_zone = strtol (optarg, &end, 10);
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
	case 'v':
	  verbose = true;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

        }
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  if (chdir (PATH_SYS_ACPI_THERMAL) < 0)
    plugin_error (STATE_UNKNOWN, 0,
		  "no ACPI thermel support in kernel, or incorrect path (\"%s\")",
		  PATH_SYS_ACPI_THERMAL);

  if ((d = opendir (".")) == NULL)
    goto error;

  unsigned long max_temp = 0, temp = 0;
  int thermal_zone = -1;
  char *scale, *type = NULL;
  double real_temp;

  while ((de = readdir (d)))
    {
      /* ignore directory entries */
      if (!strcmp (de->d_name, ".") || !strcmp (de->d_name, ".."))
	continue;

      if ((selected_thermal_zone >= 0) &&
	  (selected_thermal_zone !=
	     atoi (de->d_name + strlen ("thermal_zone"))))
	continue;

      if (!strncmp (de->d_name, "thermal_zone", strlen ("thermal_zone")))
	{
	  /* temperatures are stored in the files 
	   *  /sys/class/thermal/thermal_zone[0-9}/temp	  */
	  temp = sysfsparser_getvalue (PATH_SYS_ACPI_THERMAL "/%s/temp",
				       de->d_name);
	  type = sysfsparser_getline (PATH_SYS_ACPI_THERMAL "/%s/type",
				      de->d_name);

	  /* FIXME: as a 1st step we get the highest temp
	   *        reported by sysfs */
	  if (temp > 0)
	    {
	      found_data = true;
	      if (max_temp < temp)
		{
		  max_temp = temp;
		  thermal_zone = atoi (de->d_name + strlen ("thermal_zone"));
		}
	    }
	  if (verbose)
	    printf ("found a thermal information: %luC, thermal zone %d\n",
		    max_temp / 1000, thermal_zone);
	}
    }
  closedir (d);

  if (found_data == false)
    goto error;

  real_temp = get_real_temp (max_temp, &scale, temperature_unit);

  /* note that the critical temperature is provided by sysfs
   * ex:  cat /sys/class/thermal/thermal_zone0/trip_point_0_temp
   *      98000
   *      cat /sys/class/thermal/thermal_zone0/trip_point_0_type
   *      critical	*/

  int crit_temp = get_critical_trip_point (thermal_zone);

  status = get_status (real_temp, my_threshold);
  free (my_threshold);

  printf ("%s %s - %.1f %s (thermal zone %d, type: %s) | temp=%u%c",
	  program_name_short, state_text (status), real_temp, scale,
	  thermal_zone, type ? type : "n/a", (unsigned int) real_temp,
	  (temperature_unit == TEMP_KELVIN) ? 'K' :
	  (temperature_unit == TEMP_FAHRENHEIT) ? 'F' : 'C');
  if (crit_temp > 0)
    printf (";0;%d", crit_temp);
  putchar ('\n');

  return status;

error:
  if (selected_thermal_zone)
    plugin_error (STATE_UNKNOWN, 0,
		  "no thermal information for zone %d", selected_thermal_zone);
  else
    plugin_error (STATE_UNKNOWN, errno,
		  "no kernel support for ACPI thermal infomations");
}
