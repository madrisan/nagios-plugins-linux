// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that checks Linux Pressure Stall Information (PSI) data.
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
#include "messages.h"
#include "pressure.h"
#include "progname.h"
#include "progversion.h"
#include "system.h"
#include "thresholds.h"
#include "xasprintf.h"
#include "xstrton.h"

static const char *program_copyright =
  "Copyright (C) 2020 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "cpu", no_argument, NULL, 'C'},
  {(char *) "full", no_argument, NULL, 'f'},
  {(char *) "io", no_argument, NULL, 'i'},
  {(char *) "memory", no_argument, NULL, 'm'},
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
  fputs ("This plugin checks Linux Pressure Stall Information (PSI) data.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s --cpu [-w COUNTER] [-c COUNTER] [delay]\n", program_name);
  fprintf (out, "  %s --io [--full] [-w COUNTER] [-c COUNTER] [delay]\n",
	   program_name);
  fprintf (out, "  %s --memory [--full] [-w COUNTER] [-c COUNTER] [delay]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -C, --cpu       return the cpu pressure metrics\n", out);
  fputs ("  -i, --io        return the io (block layer/filesystems) pressure "
	 "metrics\n", out);
  fputs ("  -m, --memory    return the memory pressure metrics\n", out);
  fputs ("  -f, --full      select the data labeled \"full\" to calculate "
	 "thresholds\n", out);
  fputs ("                  instead of the one with \"some\" "
	 "(io and memory only)\n", out);
  fputs ("  -w, --warning COUNTER   warning threshold (in microseconds/s)\n",
	 out);
  fputs ("  -c, --critical COUNTER   critical threshold (in microseconds/s)\n",
	 out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  \"delay\" is the delay in seconds between two proc reads "
	   "(default: %dsec)\n", DELAY_DEFAULT);
  fputs (USAGE_NOTE, out);
  fputs ("  It requires at least a kernel 4.20, which provides this "
         "information in the\n"
         "  /proc/pressure folder.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --cpu\n", program_name);
  fprintf (out, "  %s --io\n", program_name);
  fprintf (out, "  %s --memory --full 100 2\n", program_name);
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
  bool threshold_full = false;
  int c;
  char *critical = NULL, *warning = NULL,
       *status_msg, *perfdata_mem_msg, *prefix = NULL;
  enum linux_psi_id pressure_mode = LINUX_PSI_NONE;
  unsigned long delay;
  unsigned long long starvation[2];
  struct proc_psi_oneline *psi_cpu = NULL;
  struct proc_psi_twolines *psi = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;
  int (*proc_psi_read) (struct proc_psi_twolines **,
		        unsigned long long *, unsigned long) = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "Cimfc:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'C':
	  pressure_mode = LINUX_PSI_CPU;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'f':
	  threshold_full = true;
	  break;
	case 'i':
	  prefix = "io";
	  pressure_mode = LINUX_PSI_IO;
	  proc_psi_read = &proc_psi_read_io;
	  break;
	case 'm':
	  prefix = "mem";
	  pressure_mode = LINUX_PSI_MEMORY;
	  proc_psi_read = &proc_psi_read_memory;
	  break;
	case 'w':
	  warning = optarg;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  if ((LINUX_PSI_NONE == pressure_mode) ||
      (LINUX_PSI_CPU == pressure_mode && threshold_full))
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  delay = DELAY_DEFAULT;
  if (optind < argc)
    {
      delay = strtol_or_err (argv[optind++], "failed to parse argument");

      if (delay < 1)
        plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (DELAY_MAX < delay)
        plugin_error (STATE_UNKNOWN, 0,
                      "too large delay value (greater than %d)", DELAY_MAX);
    }

  switch (pressure_mode)
  {
    default:
      usage (stderr);
    case LINUX_PSI_CPU:
      proc_psi_read_cpu (&psi_cpu, &starvation[0], delay);

      status = get_status (starvation[0], my_threshold);

      status_msg =
	xasprintf ("%s (CPU starvation) %s: %llu microsecs/s",
		   program_name_short, state_text (status), starvation[0]);
      perfdata_mem_msg =
	xasprintf ("cpu_avg10=%2.2lf%% cpu_avg60=%2.2lf%% cpu_avg300=%2.2lf%% "
		   "cpu_starvation/s=%llu"
		   , psi_cpu->avg10
		   , psi_cpu->avg60
		   , psi_cpu->avg300
		   , starvation[0]);
      break;
    case LINUX_PSI_IO:
    case LINUX_PSI_MEMORY:
      proc_psi_read (&psi, &starvation[0], delay);
      status = get_status (threshold_full ? starvation[1] : starvation[0],
			   my_threshold);
      status_msg =
	xasprintf ("%s (%s starvation) %s: some:%llu full:%llu microsecs/s"
		   , program_name_short
		   , pressure_mode == LINUX_PSI_IO ? "IO" : "Memory"
		   , state_text (status)
		   , starvation[0], starvation[1]);
      perfdata_mem_msg =
	xasprintf ("%s_some_avg10=%2.2lf%% %s_some_avg60=%2.2lf%% "
		   "%s_some_avg300=%2.2lf%% %s_some_starvation/s=%llu "
		   "%s_full_avg10=%2.2lf%% %s_full_avg60=%2.2lf%% "
		   "%s_full_avg300=%2.2lf%% %s_full_starvation/s=%llu "
		   , prefix, psi->some_avg10
		   , prefix, psi->some_avg60
		   , prefix, psi->some_avg300
		   , prefix, starvation[0]
		   , prefix, psi->full_avg10
		   , prefix, psi->full_avg60
		   , prefix, psi->full_avg300
		   , prefix, starvation[1]);
      break;
  }

  printf ("%s | %s\n", status_msg, perfdata_mem_msg);
  free (my_threshold);

  return status;
}
