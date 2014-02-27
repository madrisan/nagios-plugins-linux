/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that ckecks for the number of logged on users
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

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <utmpx.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "thresholds.h"

static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">";

static void attribute_noreturn
print_version (void)
{
  printf ("%s, version %s\n%s\n", program_name, program_version,
	  program_copyright);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
}

static struct option const longopts[] = {
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static void attribute_noreturn
usage (FILE * out)
{
  fprintf (out, "%s, version %s - "
           "display the number of users that are currently logged on.\n",
           program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out, "Usage: %s -w PERC -c PERC\n", program_name);
  fprintf (out, "       %s --help\n", program_name);
  fprintf (out, "       %s --Version\n\n", program_name);
  fputs ("\
Options:\n\
  -w, --warning PERCENT   warning threshold\n\
  -c, --critical PERCENT   critical threshold\n", out);
  fputs (HELP_OPTION_DESCRIPTION, out);
  fputs (VERSION_OPTION_DESCRIPTION, out);
  fprintf (out, "\n\
Examples:\n\
  %s -w 1\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

int
main (int argc, char **argv)
{
  int c, numuser;
  char *critical = NULL, *warning = NULL;
  thresholds *my_threshold = NULL;
  struct utmpx *utmpxstruct;
  enum nagios_status status = STATE_OK;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:w" GETOPT_HELP_VERSION_STRING,
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

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR}

    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  numuser = 0;
  setutxent ();
  while ((utmpxstruct = getutxent ()))
    {
      if ((utmpxstruct->ut_type == USER_PROCESS) &&
	  (utmpxstruct->ut_user[0] != '\0'))
	numuser++;
    }
  endutxent ();

  status = get_status (numuser, my_threshold);
  free (my_threshold);

  printf ("USERS %s - %d user%s logged on | logged_users=%d\n",
	  state_text (status), numuser, (numuser == 1 ) ? "" : "s", numuser);
}
