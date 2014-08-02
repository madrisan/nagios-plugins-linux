/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory and swap paging on Linux.
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
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "vminfo.h"
#include "xasprintf.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "paging", no_argument, NULL, 'p'},
  {(char *) "swapping", no_argument, NULL, 's'},
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
  fputs ("This plugin checks the memory and swap paging.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-p] [-s] -w PERC -c PERC\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -p, --paging    display the page reads and writes\n", out);
  fputs ("  -s, --swapping  display the swap reads amd writes\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --paging --swapping -w 10 -c 25\n", program_name);

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
  bool show_swapping = false;
  int c, status, err;
  char *critical = NULL, *warning = NULL;
  char *status_msg;
  char *perfdata_paging_msg, *perfdata_swapping_msg = NULL;
  unsigned int i, tog = 0, sleep_time = 1;
  thresholds *my_threshold = NULL;

  struct proc_vmem *vmem = NULL;
  unsigned long dpgpgin, dpgpgout, dpgfault, dpgfree, dpgmajfault,
    dpgscand, dpgscank, dpgsteal;
  unsigned long nr_vmem_pgpgin[2], nr_vmem_pgpgout[2];
  unsigned long nr_vmem_pgfault[2];
  unsigned long nr_vmem_pgfree[2];
  unsigned long nr_vmem_pgmajfault[2];
  unsigned long nr_vmem_pgsteal[2];
  unsigned long nr_vmem_pgscand[2];
  unsigned long nr_vmem_pgscank[2];
  unsigned long dpswpin, dpswpout;
  unsigned long nr_vmem_dpswpin[2], nr_vmem_dpswpout[2];
  unsigned long *tracked_value = &dpgmajfault;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "psc:w:bkmg" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'p':
	  /* show_paging = true; */
	  break;
	case 's':
	  show_swapping = true;
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

  err = proc_vmem_new (&vmem);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  for (i = 0; i < 2; i++)
    {
      proc_vmem_read (vmem);

      nr_vmem_pgpgin[tog] = proc_vmem_get_pgpgin (vmem);
      nr_vmem_pgpgout[tog] = proc_vmem_get_pgpgout (vmem);
      nr_vmem_pgfault[tog] = proc_vmem_get_pgfault (vmem);
      nr_vmem_pgmajfault[tog] = proc_vmem_get_pgmajfault (vmem);
      nr_vmem_pgfree[tog] = proc_vmem_get_pgfree (vmem);
      nr_vmem_pgsteal[tog] = proc_vmem_get_pgsteal (vmem);

      if (show_swapping)
	{
	  nr_vmem_dpswpin[tog] = proc_vmem_get_pswpin (vmem);
	  nr_vmem_dpswpout[tog] = proc_vmem_get_pswpout (vmem);
	}

      nr_vmem_pgscand[tog] = proc_vmem_get_pgscand (vmem);
      nr_vmem_pgscank[tog] = proc_vmem_get_pgscank (vmem);

      sleep (sleep_time);
      tog = !tog;
    }

  dpgpgin = nr_vmem_pgpgin[1] - nr_vmem_pgpgin[0];
  dpgpgout = nr_vmem_pgpgout[1] - nr_vmem_pgpgout[0];
  dpgfault = nr_vmem_pgfault[1] - nr_vmem_pgfault[0];
  dpgmajfault = nr_vmem_pgmajfault[1] - nr_vmem_pgmajfault[0];
  dpgfree = nr_vmem_pgfree[1] - nr_vmem_pgfree[0];
  dpgsteal = nr_vmem_pgsteal[1] - nr_vmem_pgsteal[0];
  dpgscand = nr_vmem_pgscand[1] - nr_vmem_pgscand[0];
  dpgscank = nr_vmem_pgscank[1] - nr_vmem_pgscank[0];

  if (show_swapping)
    {
      dpswpin = nr_vmem_dpswpin[1] - nr_vmem_dpswpin[0];
      dpswpout = nr_vmem_dpswpout[1] - nr_vmem_dpswpout[0];

      perfdata_swapping_msg =
	xasprintf (", vmem_pswpin/s=%lu, vmem_pswpout/s=%lu", dpswpin,
		   dpswpout);
    }

  proc_vmem_unref (vmem);

  status = get_status (*tracked_value, my_threshold);
  free (my_threshold);

  status_msg =
    xasprintf ("%s: %lu majfault/s", state_text (status), *tracked_value);
  perfdata_paging_msg =
    xasprintf ("vmem_pgpgin/s=%lu, vmem_pgpgout/s=%lu, vmem_pgfault/s=%lu, "
	       "vmem_pgmajfault/s=%lu, vmem_pgfree/s=%lu, vmem_pgsteal/s=%lu, "
	       "vmem_pgscand/s=%lu, vmem_pgscank/s=%lu",
	       dpgpgin, dpgpgout, dpgfault, dpgmajfault, dpgfree, dpgsteal,
	       dpgscand, dpgscank);

  printf ("%s %s | %s%s\n", program_name_short, status_msg, perfdata_paging_msg,
	  perfdata_swapping_msg ? perfdata_swapping_msg : "");

  return status;
}
