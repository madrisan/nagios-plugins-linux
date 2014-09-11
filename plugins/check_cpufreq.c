/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check the CPU frequency characteristics.
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
 * This software is based on the source code of the tool "vmstat".
 */

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpudesc.h"
#include "cpufreq.h"
#include "cpustats.h"
#include "cputopology.h"
#include "logging.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "system.h"
#include "xalloc.h"
#include "xasprintf.h"
#include "xstrtol.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "no-cpu-model", no_argument, NULL, 'm'},
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
  fputs ("This plugin displays the CPU frequency characteristics.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-m] [-w PERC] [-c PERC]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -m, --no-cpu-model  "
	 "do not display the CPU model in the output message\n", out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -m -w 800000:\n", program_name);

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
  int c, err;
  bool cpu_model;
  char *critical = NULL, *warning = NULL;
  nagstatus currstatus, status = STATE_OK;
  thresholds *my_threshold = NULL;
  struct cpu_desc *cpudesc = NULL;
  unsigned long freq_min, freq_max, freq_kernel;

  set_program_name (argv[0]);

  err = cpu_desc_new (&cpudesc);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  /* default values */
  cpu_model = true;

  while ((c = getopt_long (
		argc, argv, "c:w:m"
		GETOPT_HELP_VERSION_STRING, longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'm':
	  cpu_model = false;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  int ncpus = get_processor_number_total ();

  cpu_desc_read (cpudesc);
  char *cpu_model_str =
    cpu_model ?	xasprintf ("(%s) ",
			   cpu_desc_get_model_name (cpudesc)) : NULL;

  for (c = 0; c < ncpus; c++)
    if ((freq_kernel = cpufreq_get_freq_kernel (c)) > 0)
      {
	currstatus = get_status (freq_kernel, my_threshold);
	if (currstatus > status)
	  status = currstatus;
      }

  printf ("%s %s%s |"
	  , program_name_short, cpu_model ? cpu_model_str : ""
	  , state_text (status));
  for (c = 0; c < ncpus; c++)
    {
      if (0 == cpufreq_get_hardware_limits (c, &freq_min, &freq_max))
	{
	  freq_kernel = cpufreq_get_freq_kernel (c);
	  /* expected format for the Nagios performance data:
	   *   'label'=value[UOM];[warn];[crit];[min];[max]	*/
	  if (freq_kernel)
	    printf (" cpu%d_freq=%luHz;;;%lu;%lu",
		    c, freq_kernel, freq_min, freq_max);
	}
    }
  putchar ('\n');

  cpu_desc_unref (cpudesc);
  return status;
}
