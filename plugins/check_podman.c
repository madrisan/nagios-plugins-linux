// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020-2022 Davide Madrisan <davide.madrisan@gmail.com>
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
  {(char *) "block-in", no_argument, NULL, 'l'},
  {(char *) "block-out", no_argument, NULL, 'L'},
  {(char *) "cpu", no_argument, NULL, 'C'},
  {(char *) "memory", no_argument, NULL, 'M'},
  {(char *) "memory-perc", no_argument, NULL, '%'},
  {(char *) "net-in", no_argument, NULL, 'n'},
  {(char *) "net-out", no_argument, NULL, 'N'},
  {(char *) "pids", no_argument, NULL, 'p'},
  {(char *) "image", required_argument, NULL, 'i'},
  {(char *) "podman-socket", required_argument, NULL, 'p'},
  {(char *) "byte", no_argument, NULL, 'b'},
  {(char *) "kilobyte", no_argument, NULL, 'k'},
  {(char *) "megabyte", no_argument, NULL, 'm'},
  {(char *) "gigabyte", no_argument, NULL, 'g'},
  {(char *) "perc", no_argument, NULL, '%'},
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
  fputs ("This plugin returns some runtime metrics of Podman containers\n",
	 out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [--image IMAGE] [-w COUNTER] [-c COUNTER]\n",
	   program_name);
  fprintf (out, "  %s --block-in [-b,-k,-m,-g] [--image IMAGE] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fprintf (out, "  %s --block-out [-b,-k,-m,-g] [--image IMAGE] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fprintf (out, "  %s --cpu [--image IMAGE] [-w COUNTER] [-c COUNTER]\n",
          program_name);
  fprintf (out, "  %s --memory [-b,-k,-m,-g] [--image IMAGE] [--perc] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fprintf (out, "  %s --net-in [-b,-k,-m,-g] [--image IMAGE] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fprintf (out, "  %s --net-out [-b,-k,-m,-g] [--image IMAGE] "
	   "[-w COUNTER] [-c COUNTER]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs
    ("  -i, --image IMAGE   limit the investigation only to the containers "
     "running IMAGE\n", out);
  fputs ("  -l, --block-in  return the block input metrics\n", out);
  fputs ("  -C, --cpu       return the cpu percentage metrics\n", out);
  fputs ("  -L, --net-out   return the block output metrics\n", out);
  fputs ("  -M, --memory    return the runtime memory metrics\n", out);
  fputs ("  -n, --net-in    return the network input metrics\n", out);
  fputs ("  -N, --net-out   return the network output metrics\n", out);
  fputs ("  -p, --pids      return the number of PIDs\n", out);
  fputs ("  'b,-k,-m,-g     "
	 "show output in bytes, KB (the default), MB, or GB\n", out);
  fputs ("  -%, --perc      return the percentages when possible\n", out);
  fputs
    ("  -s, --podman-socket SOCKET   podman socket "
     "(default: " PODMAN_SOCKET ")\n", out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_NOTE, out);
  fputs ("  Podman stats will not work in rootless environments that use "
	 "CGroups V1.\n", out);
  fputs ("  Rootless environments that use CGroups V2 are not able to report "
	 "statistics\n", out);
  fputs ("  about their networking usage.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 100 -c 120\n", program_name);
  fprintf (out, "  %s -a unix:/run/user/1000/podman/podman.sock -w 100 -c 120\n",
           program_name);
  fprintf (out, "  %s -i \"docker.io/library/nginx:latest\" -c 5:\n",
	   program_name);
  fprintf (out, "  %s --block-out -m\n", program_name);
  fprintf (out, "  %s --cpu\n", program_name);
  fprintf (out, "  %s --memory -k --perc\n", program_name);
  fprintf (out, "  %s --memory -m --image \"docker.io/library/nginx:latest\"\n",
	   program_name);
  fprintf (out, "  %s --net-in -k --image \"docker.io/library/redis:latest\"\n",
	   program_name);
  fprintf (out, "  %s --pids --warning 8:\n", program_name);

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
  int c,
      check_selected = 0,
      shift = k_shift;
  bool report_perc = false;
  char *image = NULL;
  char *podman_socket = NULL;
  char *critical = NULL, *warning = NULL;
  char *status_msg, *perfdata_msg;
  nagstatus status = STATE_OK;
  podman_varlink_t *pv = NULL;
  stats_type which_stats = unknown;
  thresholds *my_threshold = NULL;
  total_t total;
  unsigned int containers;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "a:c:w:vi:lLnNMpbkmg%" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 'i':
	  image = optarg;
	  break;
        case 'C':
	  which_stats = cpu_stats;
	  check_selected++;
	  break;
	case 'l':
	  which_stats = block_in_stats;
	  check_selected++;
	  break;
	case 'L':
	  which_stats = block_out_stats;
	  check_selected++;
	  break;
	case 'M':
	  which_stats = memory_stats;
	  check_selected++;
	  break;
	case 'n':
	  which_stats = network_in_stats;
	  check_selected++;
	  break;
	case 'N':
	  which_stats = network_out_stats;
	  check_selected++;
	  break;
	case 'p':
	  which_stats = pids_stats;
	  check_selected++;
	  break;
	case 's':
	  podman_socket = optarg;
	  break;
	case 'b': shift = b_shift; break;
	case 'k': shift = k_shift; break;
	case 'm': shift = m_shift; break;
	case 'g': shift = g_shift; break;
	case '%': report_perc = true;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	default:
	  usage (stderr);
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  if (1 < check_selected)
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  podman_varlink_new (&pv, varlink_address);

  if (0 <= which_stats)
    {
      podman_stats (pv, which_stats, report_perc, &total, shift, image,
		    &status_msg, &perfdata_msg);
      if (which_stats == cpu_stats)
        status = get_status (total.lf, my_threshold);
      else
        status = get_status (total.llu, my_threshold);
      printf ("%s: %s | %s\n", program_name_short
	      , status_msg
	      , perfdata_msg);
    }
  else
    {
      podman_running_containers (pv, &containers, image, &perfdata_msg);
      status = get_status (containers, my_threshold);
      status_msg = image ?
	xasprintf ("%s: %u running container(s) of type \"%s\"",
		   state_text (status), containers, image) :
	xasprintf ("%s: %u running container(s)", state_text (status),
		   containers);

      printf ("%s containers %s | %s\n", program_name_short
	      , status_msg
	      , perfdata_msg);
    }

  free (my_threshold);
  podman_varlink_unref (pv);

  return status;
}
#endif /* NPL_TESTING */
