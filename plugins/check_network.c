/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
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
#ifdef HAVE_LINUX_IF_LINK_H
# include <linux/if_link.h>
#else
# include <linux/rtnetlink.h>
#endif

#include "common.h"
#include "messages.h"
#include "netinfo.h"
#include "progname.h"
#include "progversion.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin displays some network interfaces.statistics.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s\n", program_name);

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
  int c;
  nagstatus status = STATE_OK;
  const unsigned int sleep_time = 1;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  struct iflist *ifl, *iflhead = netinfo (sleep_time);
  
  printf ("%s %s | ", program_name_short, state_text (status));
  for (ifl = iflhead; ifl != NULL; ifl = ifl->next)
    printf ("%s_txpck/s=%u %s_rxpck/s=%u %s_txbyte/s=%u %s_rxbyte/s=%u "
	    "%s_txerr/s=%u %s_rxerr/s=%u %s_txdrop/s=%u %s_rxdrop/s=%u "
	    "%s_mcast/s=%u %s_coll/s=%u ",
	    ifl->ifname, ifl->tx_packets, ifl->ifname, ifl->rx_packets,
	    ifl->ifname, ifl->tx_bytes,   ifl->ifname, ifl->rx_bytes,
	    ifl->ifname, ifl->tx_errors,  ifl->ifname, ifl->rx_errors,
            ifl->ifname, ifl->tx_dropped, ifl->ifname, ifl->rx_dropped,
            ifl->ifname, ifl->multicast,  ifl->ifname, ifl->collisions
    );
  putchar ('\n');

  freeiflist (iflhead);

  return status;
}
