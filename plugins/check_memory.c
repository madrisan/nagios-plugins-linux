// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014-2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check system memory usage on Linux.
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
 * This software takes some ideas and code from procps-3.2.8 (free).
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "meminfo.h"
#include "messages.h"
#include "perfdata.h"
#include "progname.h"
#include "progversion.h"
#include "string-macros.h"
#include "system.h"
#include "thresholds.h"
#include "units.h"
#include "vminfo.h"
#include "xalloc.h"
#include "xasprintf.h"

static const char *program_copyright =
  "Copyright (C) 2014-2022 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "available", no_argument, NULL, 'a'},
  {(char *) "caches", no_argument, NULL, 'C'},
  {(char *) "vmstats", no_argument, NULL, 's'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "byte", no_argument, NULL, 'b'},
  {(char *) "kilobyte", no_argument, NULL, 'k'},
  {(char *) "megabyte", no_argument, NULL, 'm'},
  {(char *) "gigabyte", no_argument, NULL, 'g'},
  {(char *) "units", required_argument, 0, 'u'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks the system memory utilization.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-a] [-b,-k,-m,-g] [-s] [-u UNIT] -w PERC -c PERC\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -a, --available display the free/available memory\n",
	 out);
  fputs ("  -b,-k,-m,-g     "
	 "show output in bytes, KB (the default), MB, or GB\n", out);
  fputs ("  -s, --vmstats   display the virtual memory perfdata\n", out);
  fputs ("  -u, --units     show output in the selected unit (default: KB),\n",
	 out);
  fputs ("                  choose bytes, B, kB, MB, GB, KiB, MiB, GiB\n",
	 out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_NOTE_1, out);
  fputs ("  The option '-a|--available' gives an estimation of the "
	 "available memory\n"
	 "  for starting new applications without swapping.\n", out);
  fputs ("  It requires at least a kernel 3.14, which provides this "
	 "information in\n"
	 "  /proc/meminfo (see the parameter 'MemAvailable').\n", out);
  fputs ("  A MemAvailable fall-back code is implemented for "
	 "kernels 2.6.27 and above.\n", out);
  fputs ("  For older kernels 'MemFree' is returned instead.\n", out);
  fputs (USAGE_NOTE_2, out);
  fputs ("  kB/MB/GB are still calculated as their respective binary units "
	 "due to\n", out);
  fputs ("  backward compatibility issues.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --available -w 20%%: -c 10%%:\n", program_name);
  fprintf (out, "  %s --available --units MiB -w 20%%: -c 10%%:\n", program_name);
  fprintf (out, "  %s --vmstats -w 80%% -c90%%\n", program_name);

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
  bool vmem_perfdata = false;
  int c, status, err;
  int shift = k_shift;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char *status_msg, *perfdata_mem_msg,
       *perfdata_vmem_msg = "",
       *perfdata_memavailable_msg = "",
       *perfdata_memused_msg = "";
  float mem_percent = 0;
  thresholds *my_threshold = NULL;

  struct proc_sysmem *sysmem = NULL;
  unsigned long kb_mem_main_available;
  unsigned long kb_mem_main_buffers;
  unsigned long kb_mem_main_cached;
  unsigned long kb_mem_main_free;
  unsigned long kb_mem_main_shared;
  unsigned long kb_mem_main_total;
  unsigned long kb_mem_main_used;
  unsigned long kb_mem_active;
  unsigned long kb_mem_anon_pages;
  unsigned long kb_mem_committed_as;
  unsigned long kb_mem_dirty;
  unsigned long kb_mem_inactive;

  struct proc_vmem *vmem = NULL;
  unsigned long nr_vmem_pgpgin[2];
  unsigned long nr_vmem_pgpgout[2];
  unsigned long nr_vmem_pgmajfault[2];

  /* by default we display the memory used */
  unsigned long *kb_mem_monitored = &kb_mem_main_used;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
                           "aMSCsc:w:bkmgu:" GETOPT_HELP_VERSION_STRING,
                           longopts, NULL)) != -1)
    {
      switch (c)
        {
        default:
          usage (stderr);
	case 'a':
	  kb_mem_monitored = &kb_mem_main_available;
          break;
        case 'C':
          /* does nothing, exists for compatibility */
          break;
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
        case 'u':
	  if (units)
	    free (units);

	  if (STREQ (optarg, "B") || STREQ (optarg, "bytes"))
	    shift = b_shift;
	  else if (STREQ (optarg, "kB") || STREQ (optarg, "KiB"))
	    shift = k_shift;
	  else if (STREQ (optarg, "MB") || STREQ (optarg, "MiB"))
	    shift = m_shift;
	  else if (STREQ (optarg, "GB") || STREQ (optarg, "GiB"))
	    shift = g_shift;
	  else
	    plugin_error (STATE_UNKNOWN, 0, "unit type %s not known", optarg);

	  units = xstrdup (optarg);
	  break;

        case_GETOPT_HELP_CHAR
        case_GETOPT_VERSION_CHAR

	}
    }

  if (!thresholds_expressed_as_percentages (warning, critical))
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  /* output in kilobytes by default */
  if (NULL == units)
    units = xstrdup ("kB");

  err = proc_sysmem_new (&sysmem);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  proc_sysmem_read (sysmem);

  kb_mem_active       = proc_sysmem_get_active (sysmem);
  kb_mem_anon_pages   = proc_sysmem_get_anon_pages (sysmem);
  kb_mem_committed_as = proc_sysmem_get_committed_as (sysmem);
  kb_mem_dirty        = proc_sysmem_get_dirty (sysmem);
  kb_mem_inactive     = proc_sysmem_get_inactive (sysmem);
  kb_mem_main_available = proc_sysmem_get_main_available (sysmem);
  kb_mem_main_buffers = proc_sysmem_get_main_buffers (sysmem);
  kb_mem_main_cached  = proc_sysmem_get_main_cached (sysmem);
  kb_mem_main_free    = proc_sysmem_get_main_free (sysmem);
  kb_mem_main_shared  = proc_sysmem_get_main_shared (sysmem);
  kb_mem_main_total   = proc_sysmem_get_main_total (sysmem);
  kb_mem_main_used    = proc_sysmem_get_main_used (sysmem);

  if (vmem_perfdata)
    {
      unsigned long dpgpgin, dpgpgout, dpgmajfault;

      err = proc_vmem_new (&vmem);
      if (err < 0)
	plugin_error (STATE_UNKNOWN, err, "memory exhausted");

      proc_vmem_read (vmem);
      nr_vmem_pgpgin[0] = proc_vmem_get_pgpgin (vmem);
      nr_vmem_pgpgout[0] = proc_vmem_get_pgpgout (vmem);
      nr_vmem_pgmajfault[0] = proc_vmem_get_pgmajfault (vmem);

      sleep (1);

      proc_vmem_read (vmem);
      nr_vmem_pgpgin[1] = proc_vmem_get_pgpgin (vmem);
      nr_vmem_pgpgout[1] = proc_vmem_get_pgpgout (vmem);
      nr_vmem_pgmajfault[1] = proc_vmem_get_pgmajfault (vmem);

      dpgpgin = nr_vmem_pgpgin[1] - nr_vmem_pgpgin[0];
      dpgpgout = nr_vmem_pgpgout[1] - nr_vmem_pgpgout[0];
      dpgmajfault = nr_vmem_pgmajfault[1] - nr_vmem_pgmajfault[0];

      perfdata_vmem_msg =
	xasprintf (", vmem_pageins/s=%lu, vmem_pageouts/s=%lu, "
		   "vmem_pgmajfault/s=%lu", dpgpgin, dpgpgout, dpgmajfault);
    }

  /* Note: we should perhaps implement the following tests instead:
   *  1. The Main Memory Test
   *     (MemFree + Buffers + Cached) / MemTotal < threshold
   *  2. The Page Fault Test
   *     Major pagefaults > threshold
   *
   * See. http://doc.qt.digia.com/qtextended4.4/syscust-oom.html	*/

  if (kb_mem_main_total != 0)
    mem_percent = ((*kb_mem_monitored) * 100.0 / kb_mem_main_total);
  status = get_status (mem_percent, my_threshold);

  char *mem_monitored_warning = NULL,
       *mem_monitored_critical = NULL;
  unsigned long long warning_limit, critical_limit;

  if (0 == (get_perfdata_limit_converted
      (my_threshold->warning, kb_mem_main_total, shift, &warning_limit, true)))
    mem_monitored_warning = xasprintf ("%llu", warning_limit);
  if (0 == (get_perfdata_limit_converted
	    (my_threshold->critical, kb_mem_main_total, shift,
	     &critical_limit, true)))
    mem_monitored_critical = xasprintf ("%llu", critical_limit);

  /* performance data format:
   * 'label'=value[UOM];[warn];[crit];[min];[max] */
  bool add_perfdata = (kb_mem_monitored == &kb_mem_main_available);
  perfdata_memavailable_msg =
    xasprintf ("mem_available=%llu%s;%s;%s;0;%llu",
	       UNIT_STR (kb_mem_main_available),
	       (add_perfdata
	        && mem_monitored_warning) ? mem_monitored_warning : "",
	       (add_perfdata
	        && mem_monitored_critical) ? mem_monitored_critical : "",
	       UNIT_CONVERT (kb_mem_main_total, shift));

  add_perfdata = (kb_mem_monitored == &kb_mem_main_used);
  perfdata_memused_msg =
    xasprintf ("mem_used=%llu%s;%s;%s;0;%llu",
	       UNIT_STR (kb_mem_main_used),
	       (add_perfdata
		&& mem_monitored_warning) ? mem_monitored_warning : "",
	       (add_perfdata &&
		mem_monitored_critical) ? mem_monitored_critical : "",
	       UNIT_CONVERT (kb_mem_main_total, shift));

  status_msg =
    xasprintf ("%s: %.2f%% (%llu %s) %s",
	       state_text (status), mem_percent, UNIT_STR (*kb_mem_monitored),
	       (kb_mem_monitored == &kb_mem_main_available) ?
		 "available" : "used");

  free (my_threshold);

  perfdata_mem_msg =
    xasprintf ("mem_total=%llu%s %s mem_free=%llu%s "
	       "mem_shared=%llu%s mem_buffers=%llu%s mem_cached=%llu%s %s "
	       "mem_active=%llu%s mem_anonpages=%llu%s mem_committed=%llu%s "
	       "mem_dirty=%llu%s mem_inactive=%llu%s"
	       , UNIT_STR (kb_mem_main_total)
	       , perfdata_memused_msg
	       , UNIT_STR (kb_mem_main_free)
	       , UNIT_STR (kb_mem_main_shared)
	       , UNIT_STR (kb_mem_main_buffers)
	       , UNIT_STR (kb_mem_main_cached)
	       , perfdata_memavailable_msg
	       , UNIT_STR (kb_mem_active)
	       , UNIT_STR (kb_mem_anon_pages)
	       , UNIT_STR (kb_mem_committed_as)
	       , UNIT_STR (kb_mem_dirty)
	       , UNIT_STR (kb_mem_inactive));

  printf ("%s %s | %s%s\n", program_name_short, status_msg,
	  perfdata_mem_msg, perfdata_vmem_msg);

  proc_sysmem_unref (sysmem);
  proc_vmem_unref (vmem);

  return status;
}
#endif			/* NPL_TESTING */
