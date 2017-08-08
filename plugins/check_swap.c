/*
 * License: GPLv3+
 * Copyright (c) 2014,2015 Davide Madrisan <davide.madrisan@gmail.com>
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "messages.h"
#include "meminfo.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "vminfo.h"
#include "xalloc.h"
#include "xasprintf.h"

static const char *program_copyright =
  "Copyright (C) 2014,2015 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "vmstats", no_argument, NULL, 's'},
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
  fputs ("This plugin checks the swap utilization.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-b,-k,-m,-g] [-s] -w PERC -c PERC\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -b,-k,-m,-g     "
	 "show output in bytes, KB (the default), MB, or GB\n", out);
  fputs ("  -s, --vmstats   display the virtual memory perfdata\n", out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --vmstats -w 30%% -c 50%%\n", program_name);

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
  bool vmem_perfdata = false;
  int c, status, err;
  int shift = k_shift;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char *status_msg;
  char *perfdata_swap_msg, *perfdata_vmem_msg = NULL;
  float percent_used = 0;
  thresholds *my_threshold = NULL;

  struct proc_sysmem *sysmem = NULL;
  unsigned long kb_swap_cached;
  unsigned long kb_swap_free;
  unsigned long kb_swap_total;
  unsigned long kb_swap_used;

  struct proc_vmem *vmem = NULL;
  unsigned long kb_swap_pageins[2];
  unsigned long kb_swap_pageouts[2];

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "sc:w:bkmg" GETOPT_HELP_VERSION_STRING,
                           longopts, NULL)) != -1)
    {
      switch (c)
        {
        default:
          usage (stderr);
        case 's':
          vmem_perfdata = true;
          break;
        case 'c':
          critical = optarg;
          break;
        case 'w':
          warning = optarg;
          break;
        case 'b': shift = b_shift; units = xstrdup ("B"); break;
        case 'k': shift = k_shift; units = xstrdup ("kB"); break;
        case 'm': shift = m_shift; units = xstrdup ("MB"); break;
        case 'g': shift = g_shift; units = xstrdup ("GB"); break;

        case_GETOPT_HELP_CHAR
        case_GETOPT_VERSION_CHAR

        }
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  /* output in kilobytes by default */
  if (units == NULL)
    units = xstrdup ("kB");

  err = proc_sysmem_new (&sysmem);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  proc_sysmem_read (sysmem);

  kb_swap_cached = proc_sysmem_get_swap_cached (sysmem);
  kb_swap_free = proc_sysmem_get_swap_free (sysmem);
  kb_swap_total = proc_sysmem_get_swap_total (sysmem);
  kb_swap_used = proc_sysmem_get_swap_used (sysmem);

  if (vmem_perfdata)
    {
      unsigned long dpswpin, dpswpout;

      err = proc_vmem_new (&vmem);
      if (err < 0)
        plugin_error (STATE_UNKNOWN, err, "memory exhausted");

      proc_vmem_read (vmem);
      kb_swap_pageins[0] = proc_vmem_get_pswpin (vmem);
      kb_swap_pageouts[0] = proc_vmem_get_pswpout (vmem);

      sleep (1);

      proc_vmem_read (vmem);
      kb_swap_pageins[1] = proc_vmem_get_pswpin (vmem);
      kb_swap_pageouts[1] = proc_vmem_get_pswpout (vmem);

      dpswpin = kb_swap_pageins[1] - kb_swap_pageins[0];
      dpswpout = kb_swap_pageouts[1] - kb_swap_pageouts[0];

      perfdata_vmem_msg =
	xasprintf (", swap_pageins/s=%lu swap_pageouts/s=%lu",
		   dpswpin, dpswpout);
    }

  if (kb_swap_total != 0)
    percent_used = (kb_swap_used * 100.0 / kb_swap_total);

  status = get_status (percent_used, my_threshold);
  free (my_threshold);

  status_msg = xasprintf ("%s: %.2f%% (%Lu %s) used", state_text (status),
			  percent_used, UNIT_STR (kb_swap_used));

  perfdata_swap_msg =
    xasprintf ("swap_total=%Lu%s swap_used=%Lu%s swap_free=%Lu%s "
	       /* The amount of swap, in kB, used as cache memory */
	       "swap_cached=%Lu%s",
	       UNIT_STR (kb_swap_total), UNIT_STR (kb_swap_used),
	       UNIT_STR (kb_swap_free), UNIT_STR (kb_swap_cached));

  printf ("%s %s | %s%s\n", program_name_short, status_msg, perfdata_swap_msg,
	  vmem_perfdata ? perfdata_vmem_msg : "");

  proc_vmem_unref (vmem);
  proc_sysmem_unref (sysmem);

  return status;
}
