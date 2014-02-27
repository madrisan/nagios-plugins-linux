/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check the swap usage on unix.
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "messages.h"
#include "meminfo.h"
#include "progname.h"
#include "thresholds.h"
#include "xasprintf.h"

static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">";

static void attribute_noreturn usage (FILE * out)
{
  fprintf (out,
           "%s, version %s - check swap usage.\n",
           program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out,
           "Usage: %s [-b,-k,-m,-g] -w PERC -c PERC\n",
           program_name);
  fprintf (out, "       %s --help\n", program_name);
  fprintf (out, "       %s --version\n\n", program_name);
  fputs ("\
Options:\n\
  -b,-k,-m,-g     show output in bytes, KB (the default), MB, or GB\n\
  -w, --warning PERCENT   warning threshold\n\
  -c, --critical PERCENT   critical threshold\n", out);
  fputs (HELP_OPTION_DESCRIPTION, out);
  fputs (VERSION_OPTION_DESCRIPTION, out);
  fprintf (out, "\n\
Examples:\n\
  %s -w 30%% -c 50%%\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void attribute_noreturn print_version (void)
{
  printf ("%s, version %s\n%s\n", program_name, program_version,
          program_copyright);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
}

static struct option const longopts[] = {
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

extern unsigned long kb_swap_used;
extern unsigned long kb_swap_total;
extern unsigned long kb_swap_free;
extern unsigned long kb_swap_cached;
extern unsigned long kb_swap_pageins;
extern unsigned long kb_swap_pageouts;

char *
get_swap_status (int status, float percent_used, int shift,
                 const char *units)
{
  char *msg;

  msg = xasprintf ("%s: %.2f%% (%lu kB) used", state_text (status),
                   percent_used, kb_swap_used);

  return msg;
}

char *
get_swap_perfdata (int shift, const char *units)
{
  char *msg;

  msg = xasprintf ("swap_total=%Lu%s, swap_used=%Lu%s, swap_free=%Lu%s, "
                   /* The amount of swap, in kB, used as cache memory */
                   "swap_cached=%Lu%s, "
                   "swap_pageins=%Lu%s, swap_pageouts=%Lu%s\n",
                   SU (kb_swap_total), SU (kb_swap_used), SU (kb_swap_free),
                   SU (kb_swap_cached),
                   SU (kb_swap_pageins), SU (kb_swap_pageouts));

  return msg;
}

int
main (int argc, char **argv)
{
  int c, status;
  int shift = 10;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char *status_msg;
  char *perfdata_msg;
  thresholds *my_threshold = NULL;
  float percent_used = 0;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "c:w:bkmg" GETOPT_HELP_VERSION_STRING,
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

  swapinfo ();

  if (kb_swap_total != 0)
    percent_used = (kb_swap_used * 100.0 / kb_swap_total);

  status = get_status (percent_used, my_threshold);
  free (my_threshold);

  status_msg = get_swap_status (status, percent_used, shift, units);
  perfdata_msg = get_swap_perfdata (shift, units);

  printf ("%s | %s\n", status_msg, perfdata_msg);

  free (units);
  free (status_msg);
  free (perfdata_msg);

  return status;
}
