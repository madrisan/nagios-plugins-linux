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
#include "vminfo.h"
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
  int c, status, err;
  int shift = k_shift;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char *status_msg;
  char *perfdata_msg;
  float percent_used = 0;
  thresholds *my_threshold = NULL;

  struct proc_sysmem *sysmem = NULL;
  unsigned long kb_mem_main_buffers;
  unsigned long kb_mem_main_cached;
  unsigned long kb_mem_main_free;
  unsigned long kb_mem_main_shared;
  unsigned long kb_mem_main_total;
  unsigned long kb_mem_main_used;
  unsigned long kb_mem_active;
  unsigned long kb_mem_committed_as;
  unsigned long kb_mem_dirty;
  unsigned long kb_mem_inactive;

  unsigned long dpgpgin, dpgpgout, dpgmajfault;
  unsigned long kb_vmem_pgpgin[2];
  unsigned long kb_vmem_pgpgout[2];
  unsigned long nr_vmem_pgmajfault[2];

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
        case 'b': shift = b_shift; units = strdup ("B"); break;
        case 'k': shift = k_shift; units = strdup ("kB"); break;
        case 'm': shift = m_shift; units = strdup ("MB"); break;
        case 'g': shift = g_shift; units = strdup ("GB"); break;

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

  err = proc_sysmem_new (&sysmem);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  proc_sysmem_read (sysmem);

  kb_mem_committed_as = proc_sysmem_get_committed_as (sysmem);
  kb_mem_dirty        = proc_sysmem_get_dirty (sysmem);
  kb_mem_active       = proc_sysmem_get_active (sysmem);
  kb_mem_inactive     = proc_sysmem_get_inactive (sysmem);
  kb_mem_main_buffers = proc_sysmem_get_main_buffers (sysmem);
  kb_mem_main_cached  = proc_sysmem_get_main_cached (sysmem);
  kb_mem_main_free    = proc_sysmem_get_main_free (sysmem);
  kb_mem_main_shared  = proc_sysmem_get_main_shared (sysmem);
  kb_mem_main_total   = proc_sysmem_get_main_total (sysmem);
  kb_mem_main_used    = proc_sysmem_get_main_used (sysmem);

  if (cache_is_free)
    {
      kb_mem_main_used -= (kb_mem_main_cached + kb_mem_main_buffers);
      kb_mem_main_free += (kb_mem_main_cached + kb_mem_main_buffers);
    }

  proc_vmem_get_disk_io (kb_vmem_pgpgin, kb_vmem_pgpgout);
  nr_vmem_pgmajfault[0] = proc_vmem_get_pgmajfault (sysmem);

  sleep (1);

  proc_vmem_get_disk_io (kb_vmem_pgpgin + 1, kb_vmem_pgpgout + 1);
  nr_vmem_pgmajfault[1] = proc_vmem_get_pgmajfault (sysmem);

  dpgpgin = kb_vmem_pgpgin[1] - kb_vmem_pgpgin[0];
  dpgpgout = kb_vmem_pgpgout[1] - kb_vmem_pgpgout[0];
  dpgmajfault = nr_vmem_pgmajfault[1] - nr_vmem_pgmajfault[0];

  /* Note: we should perhaps implement the following tests instead: 
   *  1. The Main Memory Test
   *     (MemFree + Buffers + Cached) / MemTotal < threshold
   *  2. The Page Fault Test 
   *     Major pagefaults > threshold 
   *
   * See. http://doc.qt.digia.com/qtextended4.4/syscust-oom.html	*/

  if (kb_mem_main_total != 0)
    percent_used = (kb_mem_main_used * 100.0 / kb_mem_main_total);

  status = get_status (percent_used, my_threshold);
  free (my_threshold);

  status_msg = xasprintf ("%s: %.2f%% (%Lu %s) used", state_text (status),
			  percent_used, SU (kb_mem_main_used));
  perfdata_msg =
    xasprintf ("mem_total=%Lu%s, mem_used=%Lu%s, mem_free=%Lu%s, "
	       "mem_shared=%Lu%s, mem_buffers=%Lu%s, mem_cached=%Lu%s, "
	       "mem_active=%Lu%s, mem_committed=%Lu%s, mem_dirty=%Lu%s, "
	       "mem_inactive=%Lu%s, vmem_pageins/s=%lu, vmem_pageouts/s=%lu, "
	       "vmem_pgmajfault/s=%lu\n",
	       SU (kb_mem_main_total), SU (kb_mem_main_used),
	       SU (kb_mem_main_free), SU (kb_mem_main_shared),
	       SU (kb_mem_main_buffers), SU (kb_mem_main_cached),
	       SU (kb_mem_active), SU (kb_mem_committed_as), SU (kb_mem_dirty),
	       SU (kb_mem_inactive), dpgpgin, dpgpgout, dpgmajfault);

  printf ("%s | %s\n", status_msg, perfdata_msg);

  free (units);
  free (status_msg);
  free (perfdata_msg);
  proc_sysmem_unref (sysmem);

  return status;
}
