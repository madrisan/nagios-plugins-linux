// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2018,2024 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns some runtime metrics exposed by Docker.
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
#include <unistd.h>

#include "common.h"
#include "container.h"
#include "logging.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "units.h"
#include "xalloc.h"
#include "xasprintf.h"
#include "xstrton.h"

static const char *program_copyright =
  "Copyright (C) 2018,2024 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "image", required_argument, NULL, 'i'},
  {(char *) "memory", no_argument, NULL, 'M'},
  {(char *) "socket", required_argument, NULL, 's'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "kilobyte", no_argument, NULL, 'k'},
  {(char *) "megabyte", no_argument, NULL, 'm'},
  {(char *) "gigabyte", no_argument, NULL, 'g'},
  {(char *) "verbose", no_argument, NULL, 'v'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE *out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin returns some runtime metrics exposed by Docker\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out,
	   "  %s --socket SOCKET [--image IMAGE] [-w COUNTER] [-c COUNTER]\n",
	   program_name);
/*
  fprintf (out,
	   "  %s --socket SOCKET --memory [-k,-m,-g] [-w COUNTER] [-c COUNTER] [delay]\n",
	   program_name);  */
  fputs (USAGE_OPTIONS, out);
  fputs
    ("  -i, --image IMAGE   limit the investigation only to the containers "
     "running IMAGE\n", out);
  fputs ("  -M, --memory    check memory utilisation for running containers\n",
	 out);
  fputs ("  -s, --socket SOCKET   the path of the docker or podman socket, usually\n"
	 "                        " DOCKER_SOCKET " and " PODMAN_SOCKET "\n", out);
  fputs ("  -k,-m,-g     "
	 "show output in kB (the default), MB, or GB\n", out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between updates in seconds "
	   "(default: %dsec)\n", DELAY_DEFAULT);
  fprintf (out, "  if no socket is specified, the value of the DOCKER_HOST"
	   " environment variable will be used\n");
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  export DOCKER_HOST=\"" DOCKER_SOCKET "\"\n");
  fprintf (out, "  %s --socket /run/user/1000/podman/podman.sock\n",
	   program_name);
  fprintf (out, "  %s -w 100 -c 120\n", program_name);
/*  fprintf (out, "  %s --socket " PODMAN_SOCKET " --image nginx -c 5:\n",
	   program_name);
  fprintf (out,
	   "  %s -s /run/user/1000/podman/podman.sock --memory -m -w 512 -c 640 5\n",
	   program_name);
*/
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

#ifndef NPL_TESTING
int
main (int argc, char **argv)
{
  int c;
  int shift = k_shift;
  bool check_memory = false, verbose = false;
  char *image = NULL;
  char *socket = NULL;
  char *critical = NULL, *warning = NULL;
  char *status_msg, *perfdata_msg;
  char *units = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;
  unsigned long delay = DELAY_DEFAULT;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "i:s:Mkmgc:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	  break;
	case 'i':
	  image = optarg;
	  break;
	case 's':
	  socket = optarg;
	  break;
	case 'M':
	  check_memory = true;
	  break;
	case 'k':
	  shift = k_shift;
	  units = xstrdup ("kB");
	  break;
	case 'm':
	  shift = m_shift;
	  units = xstrdup ("MB");
	  break;
	case 'g':
	  shift = g_shift;
	  units = xstrdup ("GB");
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

  if (optind < argc)
    {
      delay = strtol_or_err (argv[optind++], "failed to parse argument");

      if (delay < 1)
	plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (DELAY_MAX < delay)
	plugin_error (STATE_UNKNOWN, 0,
		      "too large delay value (greater than %d)", DELAY_MAX);
    }

  if (check_memory && image)
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  if (check_memory)
    {
      long long unsigned int kb_memory_used_total;

      /* output in kilobytes by default */
      if (units == NULL)
	units = xstrdup ("kB");

      docker_running_containers_memory (socket, &kb_memory_used_total, verbose);
      status = get_status (UNIT_CONVERT (kb_memory_used_total, shift),
			   my_threshold);

      status_msg =
	xasprintf ("%s: %llu %s memory used", state_text (status),
		   UNIT_STR (kb_memory_used_total));
      perfdata_msg =
	xasprintf ("used=%llu%s"
		   , UNIT_STR (kb_memory_used_total));
    }
  else
    {
      unsigned int containers;
      docker_running_containers (socket, &containers, image, &perfdata_msg,
		 		 verbose);
      status = get_status (containers, my_threshold);
      status_msg = image ?
	xasprintf ("%s: %u running container(s) of type \"%s\"",
		   state_text (status), containers, image) :
	xasprintf ("%s: %u running container(s)", state_text (status),
		   containers);
    }

  printf ("%s%s %s | %s\n", program_name_short,
	  check_memory ? " memory" : " containers", status_msg, perfdata_msg);

  free (my_threshold);
  return status;
}
#endif /* NPL_TESTING */
