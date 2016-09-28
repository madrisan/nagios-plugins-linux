/*
 * License: GPLv3+
 * Copyright (c) 2014,2015 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns the number of seconds elapsed between
 * local time and Nagios time.
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
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "xstrtol.h"

static const char *program_copyright =
  "Copyright (C) 2014,2015 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "refclock", required_argument, NULL, 'r'},
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
  fputs ("This plugin returns the number of seconds elapsed between\n", out);
  fputs ("the host local time and Nagios time.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-w COUNTER] [-c COUNTER] --refclock TIME\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -r, --refclock COUNTER   the clock reference "
	 "(in seconds since the Epoch)\n",
	 out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 60 -c 120 --refclock $ARG1$\n",
	   program_name);
  fputs ("  # where $ARG1$ is the number of seconds since the Epoch: "
	 "\"$(date '+%s')\"\n", out);
  fputs ("  # provided by the Nagios poller\n", out);

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

static int
get_timedelta (unsigned long refclock, bool verbose)
{
  struct tm *tm;
  time_t t;
  char outstr[32];
  char *end = NULL;
  long timedelta;

  t = time (NULL);
  tm = localtime (&t);
  if (strftime (outstr, sizeof (outstr), "%s", tm) == 0)
    plugin_error (STATE_UNKNOWN, errno, "strftime() failed");

  timedelta = (strtol (outstr, &end, 10) - refclock);

  if (verbose)
    {
      printf ("Seconds since the Epoch: %s\n", outstr);
      printf ("Refclock: %lu  -->  Delta: %ld\n", refclock, timedelta);
    }

  return timedelta;
}

#ifndef NPL_TESTING
int
main (int argc, char **argv)
{
  int c;
  bool verbose = false;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  unsigned long refclock = ~0UL;
  long timedelta;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "r:c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'r':
	  refclock = strtol_or_err (optarg,
				    "the option '-s' requires an integer");
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

  if (refclock == ~0UL)
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  timedelta = get_timedelta (refclock, verbose);

  status = get_status (labs (timedelta), my_threshold);
  free (my_threshold);

  printf ("%s %s - time delta %lds | clock_delta=%ld\n",
	  program_name_short, state_text (status), timedelta, timedelta);

  return status;
}
#endif			/* NPL_TESTING */
