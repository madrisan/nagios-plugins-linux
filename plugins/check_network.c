// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014,2015,2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that displays some network interfaces.statistics.
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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <linux/ethtool.h>
#ifdef HAVE_LINUX_IF_LINK_H
# include <linux/if_link.h>
#else
# include <linux/rtnetlink.h>
#endif
#include <string.h>

#include "common.h"
#include "logging.h"
#include "messages.h"
#include "netinfo.h"
#include "progname.h"
#include "progversion.h"
#include "string-macros.h"
#include "system.h"
#include "thresholds.h"
#include "xalloc.h"
#include "xasprintf.h"
#include "xstrton.h"

#define MAX_PRINTED_INTERFACES		5

static const char *program_copyright =
  "Copyright (C) 2014,2015,2020 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "check-link", no_argument, NULL, 'k'},
  {(char *) "ifname", required_argument, NULL, 'i'},
  {(char *) "ifname-debug", no_argument, NULL, 0},
  {(char *) "no-bytes", no_argument, NULL, 'b'},
  {(char *) "no-collisions", no_argument, NULL, 'C'},
  {(char *) "no-drops", no_argument, NULL, 'd'},
  {(char *) "no-errors", no_argument, NULL, 'e'},
  {(char *) "no-loopback", no_argument, NULL, 'l'},
  {(char *) "no-multicast", no_argument, NULL, 'm'},
  {(char *) "no-packets", no_argument, NULL, 'p'},
  {(char *) "no-wireless", no_argument, NULL, 'W'},
  {(char *) "perc", no_argument, NULL, '%'},
  {(char *) "rx-only", no_argument, NULL, 'r'},
  {(char *) "tx-only", no_argument, NULL, 't'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

typedef enum network_check
{
  CHECK_BYTES,
  CHECK_DEFAULT = CHECK_BYTES,
  CHECK_COLLISIONS,
  CHECK_DROPPED,
  CHECK_ERRORS,
  CHECK_MULTICAST
} network_check;

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin displays some network interfaces.statistics.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [-klW] [-bCdemp] [-i <ifname-regex>] [delay]\n",
	   program_name);
  fprintf (out, "  %s [-klW] [-bCdemp] [-i <ifname-regex>] --ifname-debug\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -i, --ifname         only display interfaces matching a regular "
	 "expression\n", out);
  fputs ("      --ifname-debug   display the list of metric keys and exit\n",
	 out);
  fputs ("  -k, --check-link     report an error if at least a link is down\n",
	 out);
  fputs ("  -l, --no-loopback    skip the loopback interface\n", out);
  fputs ("  -W, --no-wireless    skip the wireless interfaces\n", out);
  fputs ("  -%, --perc           return percentage metrics if possible\n",
	 out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between the two network snapshots "
	   "in seconds (default: %dsec)\n", DELAY_DEFAULT);
  fputs ("  By default all the counters are reported in the output but it's "
	 "possible to select a subset of them:\n", out);
  fputs ("  -b  --no-bytes       omit the rx/tx bytes counter from perfdata\n",
	 out);
  fputs ("  -C, --no-collisions  omit the collisions counter from perfdata\n",
	 out);
  fputs ("  -d  --no-drops       omit the rx/tx drop counters from perfdata\n",
	 out);
  fputs ("  -e  --no-errors      omit the rx/tx errors counters "
	 "from perfdata\n", out);
  fputs ("  -m, --no-multicast   omit the multicast counter from perfdata\n",
	 out);
  fputs ("  -p, --no-packets     omit the rx/tx packets counter from "
	 "perfdata\n", out);
  fputs ("  -r, --rx-only        consider the received traffic only in "
	 "the thresholods\n", out);
  fputs ("  -t, --tx-only        consider the transmitted traffic only in "
	 "the thresholds\n", out);
  fputs (USAGE_NOTE, out);
  fputs ("  The option --ifname supports the POSIX Extended Regular Expression "
	 "syntax.\n", out);
  fputs ("  See: https://man7.org/linux/man-pages/man7/regex.7.html\n", out);
  fputs ("  You cannot select both the options r/rx-only and t/tx-only.\n",
	 out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s\n", program_name);
  fprintf (out, "  %s --check-link --ifname \"^(enp|eth)\" 15\n", program_name);
  fprintf (out, "  %s --check-link --ifname ^wlp --warning 645120 15\n",
	   program_name);
  fprintf (out, "  %s --ifname \"^(enp|wlp)\" --ifname-debug -Cdm\n",
	   program_name);
  fprintf (out, "  %s --perc --ifname \"^(enp|eth)\" -w 80%% 15\n",
	   program_name);
  fprintf (out, "  %s --no-loopback --no-wireless 15\n", program_name);

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

static inline unsigned int
get_threshold_metric (unsigned int tx, unsigned int rx,
		      bool tx_only, bool rx_only)
{
  dbg ("call to get_threshold_metric "
       "with tx:%u rx:%u, tx_only:%s, rx_only:%s\n"
       , tx, rx
       , tx_only ? "true" : "false"
       , rx_only ? "true" : "false");
  return (tx_only ? 0 : rx)
	   + (rx_only ? 0 : tx);
}

/* performance data format:
 * 'label'=value[UOM];[warn];[crit];[min];[max] */
static inline double
ratio_over_speed (unsigned int counter, unsigned long long speed)
{
  return (double)(100.0 / speed) * counter;
}

static inline char *
fmt_perfdata_bytes (const char *ifname, const char *label,
		    unsigned int counter, unsigned long long speed, bool perc)
{
  char *perfdata;

  if (perc && (speed > 0))
    {
      double counter_perc = ratio_over_speed (counter, speed);
      perfdata =
	xasprintf ("%s_%s/s=%.2f%%;;;0;100.0", ifname, label, counter_perc);
    }
  else if (!perc && (speed > 0))
    perfdata = xasprintf("%s_%s/s=%u;;;0;%llu", ifname, label, counter, speed);
  else
    perfdata = xasprintf ("%s_%s/s=%u", ifname, label, counter);

  return perfdata;
}

int
main (int argc, char **argv)
{
  int c, i, option_index = 0;
  nagstatus status;
  bool ifname_debug = false,
       pd_bytes = true,
       pd_collisions = true,
       pd_drops = true,
       pd_errors = true,
       pd_multicast = true,
       pd_packets = true,
       report_perc = false,
       rx_only = false,
       tx_only = false;
  char *p = NULL, *plugin_progname,
       *critical = NULL, *warning = NULL,
       *bp, *ifname_regex = NULL;
  size_t size;
  unsigned int options = 0;
  unsigned long delay, len;
  FILE *perfdata;
  network_check check = CHECK_DEFAULT;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "Cc:bdei:klmpWw:%" GETOPT_HELP_VERSION_STRING,
			   longopts, &option_index)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 0:
	  if (STREQ (longopts[option_index].name, "ifname-debug"))
	    ifname_debug = true;
	  break;
	case 'b':
	  pd_bytes = false;
	  break;
	case 'C':
	  pd_collisions = false;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'd':
	  pd_drops = false;
	  break;
	case 'e':
	  pd_errors = false;
	  break;
	case 'i':
	  ifname_regex = xstrdup (optarg);
	  break;
	case 'k':
	  options |= CHECK_LINK;
	  break;
	case 'l':
	  options |= NO_LOOPBACK;
	  break;
	case 'm':
	  pd_multicast = false;
	  break;
	case 'p':
	  pd_packets = false;
	  break;
	case 'r':
	  rx_only = true;
	  break;
	case 't':
	  tx_only = true;
	  break;
	case 'W':
	  options |= NO_WIRELESS;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	case '%':
	  report_perc = true;
	  break;
	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  delay = ifname_debug ? 0 : DELAY_DEFAULT;
  if (optind < argc)
    {
      if (ifname_debug)
	usage (stderr);

      delay = strtol_or_err (argv[optind++], "failed to parse argument");
      if (delay < 1)
	plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (DELAY_MAX < delay)
	plugin_error (STATE_UNKNOWN, 0,
                      "too large delay value (greater than %d)", DELAY_MAX);
    }

  if (tx_only && rx_only)
    usage (stderr);

  len = strlen (program_name);
  if (len > 6 && STRPREFIX (program_name, "check_"))
    p = (char *) program_name + 6;
  else
    plugin_error (STATE_UNKNOWN, 0,
		  "bug: the plugin does not have a standard name");

  if (STRPREFIX (p, "network_collisions"))
    {
      check = CHECK_COLLISIONS;
      if (!pd_collisions)
	usage (stderr);
      plugin_progname = xstrdup ("network collisions");
    }
  else if (STRPREFIX (p, "network_dropped"))
    {
      check = CHECK_DROPPED;
      if (!pd_drops)
	usage (stderr);
      plugin_progname = xstrdup ("network dropped");
    }
  else if (STRPREFIX (p, "network_errors"))
    {
      check = CHECK_ERRORS;
      if (!pd_errors)
	usage (stderr);
      plugin_progname = xstrdup ("network errors");
    }
  else if (STRPREFIX (p, "network_multicast"))
    {
      check = CHECK_MULTICAST;
      if (!pd_multicast)
	usage (stderr);
      plugin_progname = xstrdup ("network multicast");
    }
  else
    plugin_progname = xstrdup ("network");

  unsigned int ninterfaces;
  struct iflist *ifl, *iflhead =
    netinfo (options, ifname_regex, delay, &ninterfaces);

#define __printf_tx_rx__(metric) \
  do                                                          \
    {                                                         \
      fprintf (stdout, " - %s_tx%s\t ", ifl->ifname, metric); \
      fprintf (stdout, "%s_rx%s\n", ifl->ifname, metric);     \
    }                                                         \
  while (0)

  if (ifname_debug)
    {
      for (ifl = iflhead; ifl != NULL; ifl = ifl->next)
	{
	  bool if_up = if_flags_UP (ifl->flags),
	       if_running = if_flags_RUNNING (ifl->flags);
	  printf ("%s %s\n"
		  , ifl->ifname
		  , if_up && !if_running ? "(NO-CARRIER)"
					 : if_up ? "" : "(DOWN)");

	  if (pd_bytes)
	    __printf_tx_rx__ ("byte/s");
	  if (pd_errors)
	    __printf_tx_rx__ ("err/s");
	  if (pd_drops)
	    __printf_tx_rx__ ("drop/s");
	  if (pd_packets)
	    __printf_tx_rx__ ("pck/s");
	  if (pd_collisions)
	    fprintf (stdout, " - %s_coll/s\n", ifl->ifname);
	  if (pd_multicast)
	    fprintf (stdout, " - %s_mcast/s\n", ifl->ifname);
	}
      exit (STATE_UNKNOWN);
    }

#undef __printf_tx_rx__

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  perfdata = open_memstream (&bp, &size);
  status = STATE_OK;

  for (ifl = iflhead; ifl != NULL; ifl = ifl->next)
    {
      double counter;
      unsigned long long speed =
 	(ifl->speed > 0) ? ifl->speed * 1000*1000/8 : 0;
      nagstatus iface_status;

      /* If the output in percentages is selected and a thresholds has been
       * set, but the interface is down or its physical speed is not
       * available, make the plugin exit with an unknown state.
       * Otherwise the thresholds will be used for percentages and counters
       * which does not make sense.	*/
      if (report_perc && (warning || critical) && !(speed > 0))
	plugin_error (STATE_UNKNOWN, 0,
		      "metrics of %s cannot be converted into percentages%s"
		      , ifl->ifname
		      , if_flags_UP (ifl->flags)
			  &&if_flags_RUNNING (ifl->flags) ?
			  (0 == speed ?
			     ": physical speed is not available" : "")
			  : ": link is not UP/RUNNING");
      if (DUPLEX_HALF == ifl->duplex)
	speed /= 2;

      switch (check)
	{
	default:
	  counter = get_threshold_metric (ifl->tx_bytes, ifl->rx_bytes,
					  tx_only, rx_only);
	  if (report_perc && speed > 0)
	    counter = ratio_over_speed (counter, speed);
	  break;
	case CHECK_COLLISIONS:
	  counter = ifl->collisions;
	  break;
	case CHECK_DROPPED:
	  counter = get_threshold_metric (ifl->tx_dropped, ifl->rx_dropped,
					  tx_only, rx_only);
	  break;
	case CHECK_ERRORS:
	  counter = get_threshold_metric (ifl->tx_errors, ifl->rx_errors,
					  tx_only, rx_only);
	  break;
	case CHECK_MULTICAST:
	  counter = ifl->multicast;
	  break;
	}
      dbg ("threshold counter: %g with w:%s c:%s\n"
	   , counter
	   , warning ? warning : "unset"
	   , critical ? critical : "unset");

      iface_status = get_status (counter, my_threshold);
      if (iface_status > status)
	status = iface_status;

      if (pd_bytes)
	{
	  char *perfdata_txbyte =
	    fmt_perfdata_bytes (ifl->ifname, "txbyte", ifl->tx_bytes,
				speed, report_perc);
	  char *perfdata_rxbyte =
	    fmt_perfdata_bytes (ifl->ifname, "rxbyte", ifl->rx_bytes,
				speed, report_perc);
	  fprintf (perfdata, "%s %s ", perfdata_txbyte, perfdata_rxbyte);
	  free (perfdata_txbyte);
	  free (perfdata_rxbyte);
	}
      if (pd_errors)
	fprintf (perfdata
		, "%s_txerr/s=%u %s_rxerr/s=%u "
		, ifl->ifname, ifl->tx_errors
		, ifl->ifname, ifl->rx_errors);
      if (pd_drops)
	fprintf (perfdata
		, "%s_txdrop/s=%u %s_rxdrop/s=%u "
		, ifl->ifname, ifl->tx_dropped
		, ifl->ifname, ifl->rx_dropped);
      if (pd_packets)
        fprintf (perfdata
		 , "%s_rxpck/s=%u %s_txpck/s=%u "
		 , ifl->ifname, ifl->rx_packets
		 , ifl->ifname, ifl->tx_packets);
      if (pd_collisions)
        fprintf (perfdata, "%s_coll/s=%u "
		 , ifl->ifname, ifl->collisions);
      if (pd_multicast)
        fprintf (perfdata, "%s_mcast/s=%u "
		 , ifl->ifname, ifl->multicast);
    }

  fclose (perfdata);

  if (ninterfaces < 1)
    status = STATE_UNKNOWN;

  printf ("%s %s - found %u interface(s) ("
	  , plugin_progname
	  , state_text (status)
	  , ninterfaces);
  for (ifl = iflhead, i=0; ifl != NULL; ifl = ifl->next, i++)
    if (i < MAX_PRINTED_INTERFACES)
      printf ("%s%s", i < 1 ? "" : ",", ifl->ifname);
    else
      {
	printf (",...");
	break;
      }
  printf (") | %s\n", bp);

  freeiflist (iflhead);
  free (my_threshold);

  return status;
}
