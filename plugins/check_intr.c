/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that monitors the interrupts serviced per second,
 * including unnumbered architecture specific interrupts.
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

/* Note:  High number of interrupts per second indicates a problem with
 * hardware. It could indicate a software bug in the case of software
 * interrupts.	*/

#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "cpustats.h"
#include "interrupts.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "xstrtol.h"

/* by default one iteration with 1sec delay */
#define DELAY_DEFAULT	1
#define COUNT_DEFAULT	2

#define MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
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
  fputs ("This plugin monitors the total number of system interrupts.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-v] [-w COUNTER] -c [COUNTER] [delay [count]]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between updates in seconds "
	   "(default: %dsec)\n", DELAY_DEFAULT);
  fprintf (out, "  count is the number of updates "
	   "(default: %d)\n", COUNT_DEFAULT);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 10000 1 2\n", program_name);

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

int
main (int argc, char **argv)
{
  int c;
  bool verbose = false;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  unsigned long long nintr[2];
  unsigned int sleep_time = 1,
	   tog = 0,		/* toggle switch for cleaner code */
	   ncpus0, ncpus1;
  unsigned long i, delay, count,
	   *vintr[2] = { NULL, NULL };

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
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

  delay = DELAY_DEFAULT, count = COUNT_DEFAULT;
  if (optind < argc)
    {
      delay = strtol_or_err (argv[optind++], "failed to parse argument");

      if (delay < 1)
	plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (UINT_MAX < delay)
	plugin_error (STATE_UNKNOWN, 0, "too large delay value");

      sleep_time = delay;
    }

  if (optind < argc)
    count = strtol_or_err (argv[optind++], "failed to parse argument");

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  unsigned long long dnintr = nintr[0] = cpu_stats_get_intr ();
  if (verbose)
    printf ("intr = %Lu\n", dnintr);

  if (count <= 2)
    vintr[0] = proc_interrupts_get_nintr_per_cpu (&ncpus0);

  for (i = 1; i < count; i++)
    {
      sleep (sleep_time);

      tog = !tog;
      nintr[tog] = cpu_stats_get_intr ();

      dnintr = (nintr[tog] - nintr[!tog]) / sleep_time;
      if (verbose)
	printf ("intr = %Lu --> %Lu/s\n", nintr[tog], dnintr);

      if (count - 2 == i)
	vintr[0] = proc_interrupts_get_nintr_per_cpu (&ncpus0);
      else if (count - 1 == i)
	vintr[1] = proc_interrupts_get_nintr_per_cpu (&ncpus1);
    }

  status = get_status (dnintr, my_threshold);
  free (my_threshold);

  char *time_unit = (count > 1) ? "/s" : "";
  printf ("%s %s - number of interrupts%s %Lu | intr%s=%Lu",
	  program_name_short, state_text (status),
	  time_unit, dnintr, time_unit, dnintr);

  for (i = 0; i < MIN (ncpus0, ncpus1); i++)
    printf (" intr_cpu%lu%s=%lu", i, time_unit,
	    (count >
	     1) ? (vintr[1][i] - vintr[0][i]) / sleep_time : vintr[0][i]);
  printf ("\n");

  free (vintr[1]);
  free (vintr[0]);
}
