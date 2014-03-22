/*
 * License: GPLv3+
 * Copyright (c) 2010,2012,2013 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check how long the system has been running
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#if HAVE_SYS_SYSINFO_H
# include <sys/sysinfo.h>
#endif
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "xasprintf.h"

/* assume uptime never be zero seconds in practice */
#define UPTIME_RET_FAIL  0

static const char *program_copyright =
  "Copyright (C) 2010,2012-2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

#define BUFSIZE 0x80
static char buf[BUFSIZE + 1];

double uptime (void);
char *sprint_uptime (double);

static struct option const longopts[] = {
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
  fputs ("This plugin checks how long the system has been running.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [OPTION]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s\n", program_name);
  fprintf (out, "  %s --critical 15: --warning 30:\n", program_name);
  fputs (USAGE_SEPARATOR, out);
  fputs (USAGE_THRESHOLDS, out);

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

double
uptime ()
{
  struct sysinfo info;

  if (0 != sysinfo (&info))
    plugin_error (STATE_UNKNOWN, errno, "cannot get the system uptime");

  return info.uptime;
}

char *
sprint_uptime (double uptime_secs)
{
  int upminutes, uphours, updays;
  int pos = 0;

  updays = (int) uptime_secs / (60 * 60 * 24);
  if (updays)
    pos +=
      snprintf (buf, BUFSIZE, "%d day%s ", updays, (updays != 1) ? "s" : "");
  upminutes = (int) uptime_secs / 60;
  uphours = upminutes / 60;
  uphours = uphours % 24;
  upminutes = upminutes % 60;

  if (uphours)
    {
      pos +=
	snprintf (buf + pos, BUFSIZE - pos, "%d hour%s %d min", uphours,
		  (uphours != 1) ? "s" : "", upminutes);
    }
  else
    pos += snprintf (buf + pos, BUFSIZE - pos, "%d min", upminutes);

  return buf;
}

int
main (int argc, char **argv)
{
  int c, uptime_mins, status;
  char *critical = NULL, *warning = NULL;
  char *result_line;
  double uptime_secs;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "c:w:" GETOPT_HELP_VERSION_STRING,
                           longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
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

  if (UPTIME_RET_FAIL == (uptime_secs = uptime ()))
    plugin_error (STATE_UNKNOWN, 0, "can't get system uptime counter");

  uptime_mins = (int) uptime_secs / 60;
  status = get_status (uptime_mins, my_threshold);
  free (my_threshold);

  result_line =
    xasprintf ("%s %s: %s", program_name_short,
	       state_text (status), sprint_uptime (uptime_secs));
  printf ("%s | uptime=%d\n", result_line, uptime_mins);

  return status;
}
