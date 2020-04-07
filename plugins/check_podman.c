/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns some runtime metrics of Podman containers.
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

#include "common.h"
#include "container_podman.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "units.h"
#include "xasprintf.h"
#include "xalloc.h"

static const char *program_copyright =
  "Copyright (C) 2020 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "image", required_argument, NULL, 'i'},
  {(char *) "varlink-address", required_argument, NULL, 'a'},
  {(char *) "byte", no_argument, NULL, 'b'},
  {(char *) "verbose", no_argument, NULL, 'v'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin returns some runtime metrics of Podman containers\n",
	 out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [--image IMAGE] [-w COUNTER] [-c COUNTER]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs
    ("  -i, --image IMAGE   limit the investigation only to the containers "
     "running IMAGE\n", out);
  fputs
    ("  -a, --varlink-address ADDRESS   varlink address "
     "(default: " VARLINK_ADDRESS ")\n", out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 100 -c 120\n", program_name);
  fprintf (out, "  %s --image \"docker.io/library/nginx:latest\" -c 5:\n",
	   program_name);

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
  int c, err;
  bool verbose = false;
  char *image = NULL;
  char *varlink_address = NULL;
  char *critical = NULL, *warning = NULL;
  char *status_msg, *perfdata_msg;
  nagstatus status = STATE_OK;
  struct podman_varlink *pv = NULL;
  thresholds *my_threshold = NULL;
  unsigned int containers;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "a:c:w:vi:" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 'a':
	  varlink_address = optarg;
	  break;
	case 'i':
	  image = optarg;
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
	default:
	  usage (stderr);
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  err = podman_varlink_new (&pv, varlink_address);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  podman_running_containers (pv, &containers, image, &perfdata_msg, verbose);

  status = get_status (containers, my_threshold);
  status_msg = image ?
    xasprintf ("%s: %u running container(s) of type \"%s\"",
	       state_text (status), containers, image) :
    xasprintf ("%s: %u running container(s)", state_text (status),
	       containers);

  printf ("%s containers %s | %s\n", program_name_short,
	  status_msg, perfdata_msg);

  free (my_threshold);
  podman_varlink_unref (pv);

  return status;
}
#endif /* NPL_TESTING */
