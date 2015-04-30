/*
 * License: GPLv3+
 * Copyright (c) 2015 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that monitors the status of the fiber channel ports
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

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "sysfsparser.h"
#include "thresholds.h"

static const char *program_copyright =
  "Copyright (C) 2015 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "fchostinfo", no_argument, NULL, 'i'},
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
  fputs ("This plugin monitors the status of the fiber status ports.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s -w COUNTER -c COUNTER\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs ("  -i, --fchostinfo   show the fc_host class object attributes\n",
	 out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -c 2:\n", program_name);
  fprintf (out, "  %s --fchostinfo\n", program_name);

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

#define PATH_SYS_FC   "/sys/class"
#define PATH_SYS_FC_HOST   PATH_SYS_FC "/fc_host"

void
fc_status (int *n_ports, int *n_online, bool verbose)
{
  DIR *dirp;
  struct dirent *dp;
  char *line, path[PATH_MAX];

  // https://www.kernel.org/doc/Documentation/scsi/scsi_fc_transport.txt
  static const char *const fc_host_check[] = {
    "dev_loss_tmo",
    "fabric_name",
    "max_npiv_vports",
    "node_name",		/* The WWNN of the vport */
    "npiv_vports_inuse",
    "port_id",
    "port_name",		/* The WWPN of the vport */
    "port_state",
    "port_type",
    "speed",
    "supported_classes",
    "supported_speeds",
    "symbolic_name",
    "system_hostname",
    "tgtid_bind_type",
  };
  const int fc_host_check_len =
    sizeof fc_host_check / sizeof fc_host_check[0];

  if ((dirp = opendir (PATH_SYS_FC_HOST)) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", PATH_SYS_FC_HOST);

  /* Scan entries under /sys/class/fc_host directory */
  *n_ports = *n_online = 0;
  for (;;)
    {
      errno = 0;
      if ((dp = readdir (dirp)) == NULL)
	{
	  if (errno != 0)
	    plugin_error (STATE_UNKNOWN, errno, "readdir() failure");
	  else
	    break;		/* end-of-directory */
	}

      /* ignore directory entries */
      if (!strcmp (dp->d_name, ".") || !strcmp (dp->d_name, ".."))
	continue;

      if (dp->d_type != DT_DIR && dp->d_type != DT_LNK)
	continue;

      if (verbose)
	{
	  printf ("Class Device = \"%s\"\n", dp->d_name);
	  for (int i = 0; i < fc_host_check_len; i++)
	    {
	      snprintf (path, PATH_MAX, "%s/%s/%s",
			PATH_SYS_FC_HOST, dp->d_name, fc_host_check[i]);
	      if ((line = sysfsparser_getline ("%s", path)))
		{
		  fprintf (stdout, "%25s = \"%s\"\n", fc_host_check[i], line);
		  free (line);
		}
	    }
	  fputs ("\n", stdout);
	  continue;
	}

      (*n_ports)++;

      snprintf (path, PATH_MAX, "%s/%s/port_state",
		PATH_SYS_FC_HOST, dp->d_name);

      line = sysfsparser_getline ("%s", path);
      if (!strcmp (line, "Online"))
	(*n_online)++;

      free (line);
    }

  closedir (dirp);
}

int
main (int argc, char **argv)
{
  int c, n_ports, n_online;
  bool verbose = false;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:w:vi" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'i':
	  fc_status (&n_ports, &n_online, verbose = true);
	  return STATE_UNKNOWN;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	case 'v':
	  verbose = true;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  fc_status (&n_ports, &n_online, verbose);
  status = get_status (n_online, my_threshold);

  printf ("%s %s - Fiber Channel ports status: %d Online, %d Offline\n",
	  program_name_short, state_text (status),
	  n_online, (n_ports - n_online));
}
