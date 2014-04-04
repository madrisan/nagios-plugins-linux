/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that displays the number of running processes per user.
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

#include <sys/resource.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "messages.h"
#include "processes.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "threads", no_argument, NULL, 't'},
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
  fputs ("This plugin displays the number of running processes per user.\n",
	 out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s -w COUNTER -c COUNTER\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -t, --threads   display the number of threads\n", out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s\n", program_name);
  fprintf (out, "  %s --threads -w 1500 -c 2000\n", program_name);

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
  unsigned int nbprocs_flags = NBPROCS_NONE;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;
  struct procs_list_node *procs_list, *node;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 't':
	  nbprocs_flags |= NBPROCS_THREADS;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	case 'v':
	  nbprocs_flags |= NBPROCS_VERBOSE;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  procs_list = procs_list_getall (nbprocs_flags);

  status = get_status (procs_list_node_get_total_procs_nbr (procs_list),
		       my_threshold);
  free (my_threshold);

  printf ("%s %s - %ld running processes | ",
	  program_name_short, state_text (status),
	  procs_list_node_get_total_procs_nbr (procs_list));

  proc_list_node_foreach (node, procs_list)
#ifdef RLIMIT_NPROC
    /* 'label'=value[UOM];[warn];[crit];[min];[max] */
    printf ("nbr_%s=%ld;%lu;%lu;0 ", procs_list_node_get_username (node),
	    procs_list_node_get_nbr (node),
	    procs_list_node_get_rlimit_nproc_soft (node),
	    procs_list_node_get_rlimit_nproc_hard (node));
#else
    printf ("nbr_%s=%ld ", procs_list_node_get_username (node),
	    procs_list_node_get_nbr (node));
#endif
  putchar ('\n');
}
