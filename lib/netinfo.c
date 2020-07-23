// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014,2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for getting some network interfaces.statistics.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ethtool.h>
#include <linux/wireless.h>
#include <math.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "netinfo.h"
#include "netinfo-private.h"
#include "system.h"
#include "xasprintf.h"

extern char *const duplex_table[];

struct iflist *
netinfo (unsigned int options, const char *ifname_regex, unsigned int seconds,
	 unsigned int *ninterfaces)
{
  bool opt_check_link = (options & CHECK_LINK);
  char msgbuf[256];
  int rc;
  regex_t regex;
  struct iflist *iflhead, *ifl, *iflhead2, *ifl2;

  if ((rc =
       regcomp (&regex, ifname_regex ? ifname_regex : ".*", REG_EXTENDED)))
    {
      regerror (rc, &regex, msgbuf, sizeof (msgbuf));
      plugin_error (STATE_UNKNOWN, 0, "could not compile regex: %s", msgbuf);
    }

  dbg ("getting network informations...\n");
  iflhead = get_netinfo_snapshot (options, &regex);

  if (seconds > 0)
    {
      sleep (seconds);

      dbg ("getting network informations again (after %us)...\n", seconds);
      iflhead2 = get_netinfo_snapshot (options, &regex);

      *ninterfaces = 0;
      for (ifl = iflhead, ifl2 = iflhead2; ifl != NULL && ifl2 != NULL;
	   ifl = ifl->next, ifl2 = ifl2->next)
	{
	  if (STRNEQ (ifl->ifname, ifl2->ifname))
	    plugin_error (STATE_UNKNOWN, 0,
			  "bug in netinfo(), please contact the developers");

	  dbg ("network interface '%s'\n", ifl->ifname);

	  bool if_up = if_flags_UP (ifl->flags),
	       if_running = if_flags_RUNNING (ifl->flags);

#define DIV(a, b) ceil (((b) - (a)) / (double)seconds)
	  dbg ("\ttx_packets : %u %u\n", ifl->tx_packets, ifl2->tx_packets);
	  ifl->tx_packets = DIV (ifl->tx_packets, ifl2->tx_packets);

	  dbg ("\trx_packets : %u %u\n", ifl->rx_packets, ifl2->rx_packets);
	  ifl->rx_packets = DIV (ifl->rx_packets, ifl2->rx_packets);

	  dbg ("\ttx_bytes   : %u %u\n", ifl->tx_bytes, ifl2->tx_bytes);
	  ifl->tx_bytes   = DIV (ifl->tx_bytes, ifl2->tx_bytes);

	  dbg ("\trx_bytes   : %u %u\n", ifl->rx_bytes, ifl2->rx_bytes);
	  ifl->rx_bytes   = DIV (ifl->rx_bytes, ifl2->rx_bytes);

	  dbg ("\ttx_errors  : %u %u\n", ifl->tx_errors, ifl2->tx_errors);
	  ifl->tx_errors  = DIV (ifl->tx_errors, ifl2->tx_errors);

	  dbg ("\trx_errors  : %u %u\n", ifl->rx_errors, ifl2->rx_errors);
	  ifl->rx_errors  = DIV (ifl->rx_errors, ifl2->rx_errors);

	  dbg ("\ttx_dropped : %u %u\n", ifl->tx_dropped, ifl2->tx_dropped);
	  ifl->tx_dropped = DIV (ifl->tx_dropped, ifl2->tx_dropped);

	  dbg ("\trx_dropped : %u %u\n", ifl->rx_dropped, ifl2->rx_dropped);
	  ifl->rx_dropped = DIV (ifl->rx_dropped, ifl2->rx_dropped);

	  dbg ("\tcollisions : %u %u\n", ifl->collisions, ifl2->collisions);
	  ifl->collisions = DIV (ifl->collisions, ifl2->collisions);

	  dbg ("\tmulticast  : %u %u\n", ifl->multicast, ifl2->multicast);
	  ifl->multicast  = DIV (ifl->multicast, ifl2->multicast);
#undef DIV

	  dbg ("\tlink UP: %s\n", if_up ? "true" : "false");
	  dbg ("\tlink RUNNING: %s\n", if_running ? "true" : "false");

	  if (ifl->speed > 0)
	    dbg ("\tspeed      : %uMbit/s\n", ifl->speed);

	  if (opt_check_link && !(if_up && if_running))
	    plugin_error (STATE_CRITICAL, 0,
			  "%s matches the given regular expression "
			  "but is not UP and RUNNING!", ifl->ifname);
	  (*ninterfaces)++;
	}

      freeiflist (iflhead2);
    }

  /* Free memory allocated to the pattern buffer by regcomp() */
  regfree (&regex);

  return iflhead;
}

struct iflist *
iflist_get_next (struct iflist *ifentry)
{
  return ifentry->next;
}

/* Accessing the values from struct iflist */

const char *
iflist_get_ifname (struct iflist *ifentry)
{
  return ifentry->ifname;
}

uint8_t
iflist_get_duplex (struct iflist *ifentry)
{
  return ifentry->duplex;
}

uint32_t
iflist_get_speed (struct iflist *ifentry)
{
  return ifentry->speed;
}

#define __iflist_get__(arg) \
unsigned int iflist_get_ ## arg (struct iflist *ifentry) \
  { return ifentry->arg; }

__iflist_get__(collisions)
__iflist_get__(flags)
__iflist_get__(multicast)
__iflist_get__(tx_packets)
__iflist_get__(rx_packets)
__iflist_get__(tx_bytes)
__iflist_get__(rx_bytes)
__iflist_get__(tx_errors)
__iflist_get__(rx_errors)
__iflist_get__(tx_dropped)
__iflist_get__(rx_dropped)
#undef __iflist_get__

/* Print the list of network interfaces (for debug) */

void print_ifname_debug (struct iflist *iflhead, unsigned int options)
{
  bool pd_bytes      = (options & NO_BYTES) != NO_BYTES,
       pd_collisions = (options & NO_COLLISIONS) != NO_COLLISIONS,
       pd_drops      = (options & NO_DROPS) != NO_DROPS,
       pd_errors     = (options & NO_ERRORS) != NO_ERRORS,
       pd_multicast  = (options & NO_MULTICAST) != NO_MULTICAST,
       pd_packets    = (options & NO_PACKETS) != NO_PACKETS,
       rx_only       = options & RX_ONLY,
       tx_only       = options & TX_ONLY;
  struct iflist *ifl;

#define __printf_tx_rx__(metric, tx_only, rx_only)           \
  do                                                         \
    {                                                        \
      fprintf (stdout, " - ");                               \
      if (!rx_only)                                          \
        fprintf (stdout, "%s_tx%s\t ", ifl->ifname, metric); \
      if (!tx_only)                                          \
        fprintf (stdout, "%s_rx%s", ifl->ifname, metric);    \
      fprintf (stdout, "\n");                                \
    }                                                        \
  while (0)

  for (ifl = iflhead; ifl != NULL; ifl = ifl->next)
    {
      bool if_up = if_flags_UP (ifl->flags),
	   if_running = if_flags_RUNNING (ifl->flags);
      char *ifduplex = NULL,
	   *ifspeed = NULL;

      if (ifl->speed > 0)
	ifspeed = xasprintf (" link-speed:%uMbps", ifl->speed);
      if (ifl->duplex != _DUP_UNKNOWN)
	ifduplex = xasprintf (" %s-duplex", duplex_table[ifl->duplex]);
      printf ("%s%s%s%s\n"
	      , ifl->ifname
	      , if_up && !if_running ? " (NO-CARRIER)"
				     : if_up ? "" : " (DOWN)"
	      , ifspeed ? ifspeed : ""
	      , ifduplex ? ifduplex : "");

      if (pd_bytes)
	__printf_tx_rx__ ("byte/s", tx_only, rx_only);
      if (pd_errors)
	__printf_tx_rx__ ("err/s", tx_only, rx_only);
      if (pd_drops)
	__printf_tx_rx__ ("drop/s", tx_only, rx_only);
      if (pd_packets)
	__printf_tx_rx__ ("pck/s", tx_only, rx_only);
      if (pd_collisions)
	fprintf (stdout, " - %s_coll/s\n", ifl->ifname);
      if (pd_multicast)
	fprintf (stdout, " - %s_mcast/s\n", ifl->ifname);
    }
#undef __printf_tx_rx__
}

void
freeiflist (struct iflist *iflhead)
{
  struct iflist *ifl = iflhead, *iflnext;

  while (ifl != NULL)
    {
      iflnext = ifl->next;
      free (ifl->ifname);
      free (ifl);
      ifl = iflnext;
    }
}

/* helper functions for parsing interface flags */
#define ifa_flags_parser(arg) \
bool if_flags_ ## arg (unsigned int flags) \
  { return flags & IFF_ ## arg; }

ifa_flags_parser (LOOPBACK);
ifa_flags_parser (RUNNING);
ifa_flags_parser (UP);
