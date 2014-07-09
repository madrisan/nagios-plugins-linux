/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that tests the current system load average
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
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "cpudesc.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "system.h"
#include "xasprintf.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "load1", required_argument, NULL, '1'},
  {(char *) "load5", required_argument, NULL, '5'},
  {(char *) "load15", required_argument, NULL, 'L'},
  {(char *) "percpu", no_argument, NULL, 'r'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks the current system load average.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-r] [--load1=w,c] [--load5=w,c] [--load15=w,c]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -r, --percpu    divide the load averages by the number of CPUs\n",
	 out);
  fputs ("  -1, --load1=WLOAD1,CLOAD1"
	 "   warning and critical thresholds for load1\n", out);
  fputs ("  -5, --load5=WLOAD5,CLOAD5"
	 "   warning and critical thresholds for load5\n", out);
  fputs ("  -L, --load15=WLOAD15,CLOAD15"
	 "   warning and critical thresholds for load15\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -r --load1=2,3 --load15=1.5,2.5\n", program_name);

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

static void
validate_input (int i, double w, double c)
{
  if (i != 2 || (w >= c))
    plugin_error (STATE_UNKNOWN, 0, "command line error: bad thresholds");
}

int
main (int argc, char **argv)
{
  int c, i;
  int status = STATE_OK;
  int numcpus = 1;
  const unsigned int lamin[3] = { 1, 5, 15 };
  bool required[3] = { false, false, false };
  double loadavg[3];
  double wload[3] = { 0.0, 0.0, 0.0 };
  double cload[3] = { 0.0, 0.0, 0.0 };
  char *status_msg, *perfdata_msg;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "1:5:L:r" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case '1':
	  i = sscanf (optarg, "%lf,%lf", &wload[0], &cload[0]);
	  validate_input (i, wload[0], cload[0]);
	  required[0] = true;
	  break;
	case '5':
	  i = sscanf (optarg, "%lf,%lf", &wload[1], &cload[1]);
	  validate_input (i, wload[1], cload[1]);
	  required[1] = true;
	  break;
	case 'L':
	  i = sscanf (optarg, "%lf,%lf", &wload[2], &cload[2]);
	  validate_input (i, wload[2], cload[2]);
	  required[2] = true;
	  break;
	case 'r':
	  numcpus = get_processor_number_online ();
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  if (getloadavg (&loadavg[0], 3) != 3)
    plugin_error (STATE_UNKNOWN, 0,
		  "the system load average was unobtainable");

  for (i = 0; i < 3; i++)
    {
      if (numcpus > 1)
	loadavg[i] /= numcpus;
    }

  for (i = 0; i < 3; i++)
    {
      if (required[i] == false)
	continue;

      if (loadavg[i] > cload[i])
	{
	  status = STATE_CRITICAL;
	  break;
	}
      else if (loadavg[i] > wload[i])
	status = STATE_WARNING;
    }

  status_msg =
    xasprintf ("%s - average: %.2lf, %.2lf, %.2lf",
	       state_text (status), loadavg[0], loadavg[1], loadavg[2]);

  /* performance data format:
   *'label'=value[UOM];[warn];[crit];[min];[max]  */
  perfdata_msg =
    xasprintf ("load%d=%.3lf;%.3lf;%.3lf;0, "
	       "load%d=%.3lf;%.3lf;%.3lf;0, "
	       "load%d=%.3lf;%.3lf;%.3lf;0"
	       , lamin[0], loadavg[0], wload[0], cload[0]
	       , lamin[1], loadavg[1], wload[1], cload[1]
	       , lamin[2], loadavg[2], wload[2], cload[2]);

  printf ("%s %s | %s\n", program_name_short, status_msg, perfdata_msg);

  return status;
}
