// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014,2015,2023 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that ckecks for the number of logged on users.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined(WITH_SYSTEMD)
#include <utmpx.h>
#else
#include <systemd/sd-login.h>
#endif

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"

static const char *program_copyright =
  "Copyright (C) 2014,2015,2023 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

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
  fputs ("This plugin displays the number of users "
	 "that are currently logged on.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-w COUNTER] [-c COUNTER]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 1\n", program_name);

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
count_users (bool verbose)
{
  int numuser = 0;

#if defined(WITH_SYSTEMD)
  /* sd_get_sessions() will not return ENOENT as error, as this only means
   * that there is no session, not that systemd is not used.
   * Use the recommended sd_booted() function for this.
   */
  if (sd_booted() > 0)
    return sd_get_sessions(NULL);
#endif

  struct utmpx *ut;

  if (verbose)
    printf ("user         PID line   host      date/time\n");
  setutxent ();
  while ((ut = getutxent ()))
    {
      if ((ut->ut_type == USER_PROCESS) && (ut->ut_user[0] != '\0'))
	{
	  numuser++;
	  if (!verbose)
	    continue;
	  char buf[26];
	  time_t timetmp = ut->ut_tv.tv_sec;
	  printf ("%-8s %6ld %-6.6s %-9.9s %s", ut->ut_user,
		  (long) ut->ut_pid, ut->ut_line, ut->ut_host,
		  ctime_r (&timetmp, buf));
	}
    }
  endutxent ();

  return numuser;
}

int
main (int argc, char **argv)
{
  int c, numuser;
  bool verbose = false;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

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

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  numuser = count_users (verbose);
  if (numuser < 0)
    plugin_error (STATE_UNKNOWN, numuser,
		  "cannot determine all current login sessions");

  status = get_status (numuser, my_threshold);
  free (my_threshold);

  printf ("%s %s - %d user%s logged on | logged_users=%d\n",
	  program_name_short, state_text (status), numuser,
	  (numuser == 1 ) ? "" : "s", numuser);

  return status;
}
