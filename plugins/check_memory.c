/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory usage on unix.
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
 * This software is based on the source code of the tool "free" (procps 3.2.8)
 */

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "meminfo.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "xasprintf.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "caches", no_argument, NULL, 'C'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "byte", no_argument, NULL, 'b'},
  {(char *) "kilobyte", no_argument, NULL, 'k'},
  {(char *) "megabyte", no_argument, NULL, 'm'},
  {(char *) "gigabyte", no_argument, NULL, 'g'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks the memory utilization.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-b,-k,-m,-g] [-C] -w PERC -c PERC\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -b,-k,-m,-g     "
	 "show output in bytes, KB (the default), MB, or GB\n", out);
  fputs ("  -C, --caches    count buffers and cached memory as free memory\n",
	 out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -C -w 80%% -c90%%\n", program_name);

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
  bool cache_is_free = false;
  int c, status;
  int shift = 10;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char *status_msg;
  char *perfdata_msg;
  float percent_used = 0;
  thresholds *my_threshold = NULL;

  struct memory_snapshot *mem = NULL;
  unsigned long dpgpgin, dpgpgout;
  unsigned long kb_mem_pageins[2];
  unsigned long kb_mem_pageouts[2];

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
                           "MSCc:w:bkmg" GETOPT_HELP_VERSION_STRING,
                           longopts, NULL)) != -1)
    {
      switch (c)
        {
        default:
          usage (stderr);
        case 'C':
          cache_is_free = true;
          break;
        case 'c':
          critical = optarg;
          break;
        case 'w':
          warning = optarg;
          break;
        case 'b': shift = 0;  units = strdup ("B"); break;
        case 'k': shift = 10; units = strdup ("kB"); break;
        case 'm': shift = 20; units = strdup ("MB"); break;
        case 'g': shift = 30; units = strdup ("GB"); break;

        case_GETOPT_HELP_CHAR
        case_GETOPT_VERSION_CHAR

        }
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  /* output in kilobytes by default */
  if (units == NULL)
    units = strdup ("kB");

  get_meminfo (cache_is_free, &mem);

  get_mempaginginfo (kb_mem_pageins, kb_mem_pageouts);
  sleep (1);
  get_mempaginginfo (kb_mem_pageins + 1, kb_mem_pageouts + 1);
  dpgpgin = kb_mem_pageins[1] - kb_mem_pageins[0];
  dpgpgout = kb_mem_pageouts[1] - kb_mem_pageouts[0];

  if (mem->total != 0)
    percent_used = (mem->used * 100.0 / mem->total);

  status = get_status (percent_used, my_threshold);
  free (my_threshold);

  status_msg = xasprintf ("%s: %.2f%% (%lu kB) used", state_text (status),
			  percent_used, mem->used);
  perfdata_msg =
    xasprintf ("mem_total=%Lu%s, mem_used=%Lu%s, mem_free=%Lu%s, "
	       "mem_shared=%Lu%s, mem_buffers=%Lu%s, mem_cached=%Lu%s, "
	       "mem_pageins/s=%lu, mem_pageouts/s=%lu\n", SU (mem->total),
	       SU (mem->used), SU (mem->free), SU (mem->shared),
	       SU (mem->buffers), SU (mem->cached), dpgpgin, dpgpgout);

  printf ("%s | %s\n", status_msg, perfdata_msg);

  free (units);
  free (status_msg);
  free (perfdata_msg);
  free (mem);

  return status;
}
