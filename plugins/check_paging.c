/*
 * License: GPLv3+
 * Copyright (c) 2014,2015,2017 Davide Madrisan <davide.madrisan@gmail.com>
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
  "Copyright (C) 2014,2015,2017 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "paging", no_argument, NULL, 'p'},
  {(char *) "swapping", no_argument, NULL, 's'},
  {(char *) "swapping-only", no_argument, NULL, 'S'},
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
  fputs ("This plugin checks the memory and swap paging.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-s] [-S] [-w PAGES] [-c PAGES]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -s, --swapping  display also the swap reads and writes\n", out);
  fputs ("  -S, --swapping-only  only display the swap reads and writes\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  PAGES is the sum of `pswpin' and `pswpout' per second,\n"
		"\tif the command-line option `--swapping-only' "
		"has been specified,\n"
		"\tor the number of `majfault/s' otherwise.\n");
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --swapping -w 10 -c 25\n", program_name);
  fprintf (out, "  %s --swapping-only -w 40 -c 60\n", program_name);

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

typedef struct paging_data
{
  unsigned long dpgpgin;
  unsigned long dpgpgout;
  unsigned long dpgfault;
  unsigned long dpgfree;
  unsigned long dpgmajfault;
  unsigned long dpgscand;
  unsigned long dpgscank;
  unsigned long dpgsteal;
  unsigned long dpswpin;
  unsigned long dpswpout;
  unsigned long summary;
} paging_data_t;

static void
get_paging_status (bool show_swapping, bool swapping_only,
		   paging_data_t *paging)
{
  struct proc_vmem *vmem = NULL;
  unsigned long nr_vmem_pgpgin[2], nr_vmem_pgpgout[2];
  unsigned long nr_vmem_pgfault[2];
  unsigned long nr_vmem_pgfree[2];
  unsigned long nr_vmem_pgmajfault[2];
  unsigned long nr_vmem_pgsteal[2];
  unsigned long nr_vmem_pgscand[2];
  unsigned long nr_vmem_pgscank[2];
  unsigned long nr_vmem_dpswpin[2], nr_vmem_dpswpout[2];
  unsigned int i, tog = 0, sleep_time = 1;
  int err;

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
      nr_vmem_pgscand[tog] = proc_vmem_get_pgscand (vmem);
      nr_vmem_pgscank[tog] = proc_vmem_get_pgscank (vmem);

      nr_vmem_dpswpin[tog] = proc_vmem_get_pswpin (vmem);
      nr_vmem_dpswpout[tog] = proc_vmem_get_pswpout (vmem);

      sleep (sleep_time);
      tog = !tog;
    }

  paging->dpgpgin = nr_vmem_pgpgin[1] - nr_vmem_pgpgin[0];
  paging->dpgpgout = nr_vmem_pgpgout[1] - nr_vmem_pgpgout[0];
  paging->dpgfault = nr_vmem_pgfault[1] - nr_vmem_pgfault[0];
  paging->dpgmajfault = nr_vmem_pgmajfault[1] - nr_vmem_pgmajfault[0];
  paging->dpgfree = nr_vmem_pgfree[1] - nr_vmem_pgfree[0];
  paging->dpgsteal = nr_vmem_pgsteal[1] - nr_vmem_pgsteal[0];
  paging->dpgscand = nr_vmem_pgscand[1] - nr_vmem_pgscand[0];
  paging->dpgscank = nr_vmem_pgscank[1] - nr_vmem_pgscank[0];

  paging->dpswpin = nr_vmem_dpswpin[1] - nr_vmem_dpswpin[0];
  paging->dpswpout = nr_vmem_dpswpout[1] - nr_vmem_dpswpout[0];

  paging->summary =
    swapping_only ? (paging->dpswpin +
		     paging->dpswpout) : paging->dpgmajfault;

  proc_vmem_unref (vmem);
}

int
main (int argc, char **argv)
{
  bool show_swapping = false;
  bool swapping_only = false;
  int c, status;
  char *critical = NULL, *warning = NULL;
  char *status_msg;
  char *perfdata_paging_msg = NULL, *perfdata_swapping_msg = NULL;
  set_program_name (argv[0]);
  thresholds *my_threshold = NULL;
  paging_data_t paging;

  while ((c = getopt_long (argc, argv, "psSc:w:" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'p':
	  /* show_paging = true; left for backward compatibility */
	  break;
	case 's':
	  show_swapping = true;
	  break;
	case 'S':
	  swapping_only = true;
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

  get_paging_status (show_swapping, swapping_only, &paging);

  status = get_status (paging.summary, my_threshold);
  free (my_threshold);

  status_msg =
    xasprintf ("%s: %lu %s/s", state_text (status), paging.summary,
	       swapping_only ? "pswp" : "majfault");
  if (!swapping_only)
    perfdata_paging_msg =
      xasprintf ("vmem_pgpgin/s=%lu vmem_pgpgout/s=%lu vmem_pgfault/s=%lu "
		 "vmem_pgmajfault/s=%lu vmem_pgfree/s=%lu vmem_pgsteal/s=%lu "
		 "vmem_pgscand/s=%lu vmem_pgscank/s=%lu ",
		 paging.dpgpgin, paging.dpgpgout, paging.dpgfault,
		 paging.dpgmajfault, paging.dpgfree, paging.dpgsteal,
		 paging.dpgscand, paging.dpgscank);
  if (show_swapping || swapping_only)
    perfdata_swapping_msg =
      xasprintf ("vmem_pswpin/s=%lu vmem_pswpout/s=%lu",
		 paging.dpswpin, paging.dpswpout);

  printf ("%s %s | %s%s\n", program_name_short, status_msg,
	  perfdata_paging_msg ? perfdata_paging_msg : "",
	  perfdata_swapping_msg ? perfdata_swapping_msg : "");

  return status;
}
