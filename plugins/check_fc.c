// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2015,2022 Davide Madrisan <davide.madrisan@gmail.com>
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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "sysfsparser.h"
#include "thresholds.h"
#include "xstrton.h"

static const char *program_copyright =
  "Copyright (C) 2015,2022 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

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
  fprintf (out, "  %s -w COUNTER -c COUNTER [delay [count]]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs ("  -i, --fchostinfo   show the fc_host class object attributes\n",
	 out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between updates in seconds "
	   "(default: %dsec)\n", DELAY_DEFAULT);
  fprintf (out, "  count is the number of updates "
	   "(default: %d)\n", COUNT_DEFAULT);
  fputs ("\t1 means the total inbound/outbound traffic from boottime.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -c 2:\n", program_name);
  fprintf (out, "  %s -i -v\n", program_name);

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

#define PATH_SYS_FC  PATH_SYS "/class"
#define PATH_SYS_FC_HOST   PATH_SYS_FC "/fc_host"

void
fc_host_summary (bool verbose)
{
  DIR *dirp;
  struct dirent *dp;
  char *line, path[PATH_MAX];

  sysfsparser_opendir(&dirp, PATH_SYS_FC_HOST);

  /* Scan entries under /sys/class/fc_host directory */
  while ((dp = sysfsparser_readfilename(dirp, DT_DIR | DT_LNK)))
    {
      DIR *dirp_host;
      struct dirent *dp_host;

      printf ("Class Device = \"%s\"\n", dp->d_name);

      if (!verbose)
	continue;

      snprintf (path, PATH_MAX, "%s/%s/device", PATH_SYS_FC_HOST, dp->d_name);
      char *cdevpath = realpath (path, NULL);
      printf ("Class Device path = \"%s\"\n", cdevpath);

      sysfsparser_opendir(&dirp_host, "%s/%s", PATH_SYS_FC_HOST, dp->d_name);

      /* https://www.kernel.org/doc/Documentation/scsi/scsi_fc_transport.txt */
      while ((dp_host = sysfsparser_readfilename(dirp_host, DT_REG)))
	{
	  snprintf (path, PATH_MAX, "%s/%s/%s",
		    PATH_SYS_FC_HOST, dp->d_name, dp_host->d_name);
 	  if ((line = sysfsparser_getline ("%s", path)))
	    {
	      fprintf (stdout, "%25s = \"%s\"\n", dp_host->d_name, line);
	      free (line);
	    }
	}

      fputs ("\n", stdout);

      sysfsparser_closedir (dirp_host);
      free (cdevpath);
    }

  sysfsparser_closedir (dirp);
}

static uint64_t
fc_host_get_statistic (const char *which, const char *host)
{
  unsigned long long value;
  int err;

  err = sysfsparser_getvalue (&value, PATH_SYS_FC_HOST "/%s/statistics/%s",
			      host, which);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, 0,
		  "an error has occurred while reading "
		  PATH_SYS_FC_HOST "/%s/statistics/%s",
		  host, which);

  dbg (PATH_SYS_FC_HOST "/%s/statistics/%s = %llu\n",
       host, which, value);

  return value;
}

#define fc_get_stat(name)                         \
static uint64_t fc_stat_##name(const char *host)  \
{                                                 \
  return fc_host_get_statistic (#name, host);     \
}
fc_get_stat(rx_frames);
fc_get_stat(tx_frames);
fc_get_stat(error_frames);
fc_get_stat(invalid_crc_count);
fc_get_stat(link_failure_count);
fc_get_stat(loss_of_signal_count);
fc_get_stat(loss_of_sync_count);

/* see <http://osxr.org/linux/source/drivers/scsi/scsi_transport_fc.c> */
typedef struct fc_host_statistics {
  uint64_t rx_frames;
  uint64_t tx_frames;
  uint64_t error_frames;
  uint64_t invalid_crc_count;
  uint64_t link_failure_count;
  uint64_t loss_of_signal_count;
  uint64_t loss_of_sync_count;
} fc_host_statistics;

static void
fc_host_status (int *n_ports, int *n_online, fc_host_statistics *stats,
		unsigned int delay, unsigned int count)
{
  DIR *dirp;
  struct dirent *dp;
  char path[PATH_MAX];
  uint64_t drx_frames_tot = 0, dtx_frames_tot = 0;

  *n_ports = *n_online = 0;
  sysfsparser_opendir(&dirp, PATH_SYS_FC_HOST);

  /* Scan entries under /sys/class/fc_host directory */
  while ((dp = sysfsparser_readfilename(dirp, DT_DIR | DT_LNK)))
    {
      (*n_ports)++;
      snprintf (path, PATH_MAX, "%s/%s/port_state",
		PATH_SYS_FC_HOST, dp->d_name);

      char *line = sysfsparser_getline ("%s", path);
      if (STREQ (line, "Online"))
	(*n_online)++;

      free (line);

      /* collect some statistics */
      uint64_t rx_frames[2], tx_frames[2];
      unsigned int i, tog = 0;

      stats->rx_frames = rx_frames[0] = fc_stat_rx_frames (dp->d_name);
      stats->tx_frames = tx_frames[0] = fc_stat_tx_frames (dp->d_name);

      for (i = 1; i < count; i++)
	{
	  sleep (delay);
	  tog = !tog;

	  rx_frames[tog] = fc_stat_rx_frames (dp->d_name);
	  stats->rx_frames = rx_frames[tog] - rx_frames[!tog];
	  tx_frames[tog] = fc_stat_tx_frames (dp->d_name);
	  stats->tx_frames = tx_frames[tog] - tx_frames[!tog];
	}

	drx_frames_tot += stats->rx_frames;
	dtx_frames_tot += stats->tx_frames;

	stats->error_frames += fc_stat_error_frames (dp->d_name);
	stats->invalid_crc_count += fc_stat_invalid_crc_count (dp->d_name);
	stats->link_failure_count += fc_stat_link_failure_count (dp->d_name);
	stats->loss_of_signal_count +=
	  fc_stat_loss_of_signal_count (dp->d_name);
	stats->loss_of_sync_count += fc_stat_loss_of_sync_count (dp->d_name);
    }

  sysfsparser_closedir (dirp);

  stats->rx_frames = drx_frames_tot;
  stats->tx_frames = dtx_frames_tot;
}

#undef PATH_SYS_FC
#undef PATH_SYS_FC_HOST

int
main (int argc, char **argv)
{
  int c, n_ports, n_online;
  bool verbose = false, summary = false;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;
  unsigned long count, delay;

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
	  summary = true;
	  break;
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

  if (summary)
    {
      sysfsparser_check_for_sysfs ();
      fc_host_summary (verbose);
      return STATE_UNKNOWN;
    }

  delay = DELAY_DEFAULT, count = COUNT_DEFAULT;
  if (optind < argc)
    {
      delay = strtol_or_err (argv[optind++], "failed to parse argument");

      if (delay < 1)
	plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (DELAY_MAX < delay)
	plugin_error (STATE_UNKNOWN, 0,
		      "too large delay value (greater than %d)", DELAY_MAX);
    }

  if (optind < argc)
    {
      count = strtol_or_err (argv[optind++], "failed to parse argument");
      if (COUNT_MAX < count)
	plugin_error (STATE_UNKNOWN, 0,
		      "too large count value (greater than %d)", COUNT_MAX);
    }

  sysfsparser_check_for_sysfs ();

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  fc_host_statistics stats = {0};

  fc_host_status (&n_ports, &n_online, &stats, delay, count);
  status = get_status (n_online, my_threshold);

  printf ("%s %s - Fiber Channel ports status: %d/%d Online "
	  "| rx_frames=%llu tx_frames=%llu"
	  " error_frames=%llu"
	  " invalid_crc_count=%llu"
	  " link_failure_count=%llu"
	  " loss_of_signal_count=%llu"
	  " loss_of_sync_count=%llu\n",
	  program_name_short, state_text (status),
	  n_online, n_ports,
	  (unsigned long long) stats.rx_frames,
	  (unsigned long long) stats.tx_frames,
	  (unsigned long long) stats.error_frames,
	  (unsigned long long) stats.invalid_crc_count,
	  (unsigned long long) stats.link_failure_count,
	  (unsigned long long) stats.loss_of_signal_count,
	  (unsigned long long) stats.loss_of_sync_count
  );

  return status;
}
