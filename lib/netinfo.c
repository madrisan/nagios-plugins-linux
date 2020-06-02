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

#include <errno.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#ifdef HAVE_LINUX_IF_LINK_H
# include <linux/if_link.h>
#else
# include <linux/rtnetlink.h>
#endif
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "netinfo.h"
#include "system.h"
#include "xalloc.h"

static int
check_link (int sock, const char *ifname, bool *up, bool *running)
{
  struct ifreq ifr;

  STRNCPY_TERMINATED (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
  if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
    return -1;

  *up = (ifr.ifr_flags & IFF_UP);
  *running = (ifr.ifr_flags & IFF_RUNNING);
  return 0;
}

static bool
wireless_interface (int sock, const char *ifname)
{
  struct iwreq pwrq;
  memset (&pwrq, 0, sizeof (pwrq));
  STRNCPY_TERMINATED (pwrq.ifr_name, ifname, IFNAMSIZ);

  // int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (ioctl (sock, SIOCGIWNAME, &pwrq) != -1)
    {
      dbg ("wireless interface detected (%s): '%s'\n",
	   pwrq.u.name, ifname);
      return true;
    }

  return false;
}

static struct iflist *
get_netinfo_snapshot (unsigned int options, const regex_t *iface_regex)
{
  bool opt_ignore_loopback = (options & NO_LOOPBACK),
       opt_ignore_wireless = (options & NO_WIRELESS);
  int family, sock;
  struct ifaddrs *ifaddr, *ifa;

  if (getifaddrs (&ifaddr) == -1)
    plugin_error (STATE_UNKNOWN, errno, "getifaddrs() failed");

  sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0)
    plugin_error (STATE_UNKNOWN, errno, "socket() failed");

  struct iflist *iflhead = NULL, *iflprev = NULL, *ifl;

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_data == NULL)
	continue;

      family = ifa->ifa_addr->sa_family;
      if (family != AF_PACKET)
	continue;

      bool is_wireless = wireless_interface (sock, ifa->ifa_name);
      bool skip_interface =
	((opt_ignore_loopback && STREQ ("lo", ifa->ifa_name))
	 || (is_wireless && opt_ignore_wireless)
	 || (regexec (iface_regex, ifa->ifa_name, (size_t) 0, NULL, 0)));
      if (skip_interface)
	{
	  dbg ("ignoring network interface '%s'...\n", ifa->ifa_name);
	  continue;
	}

      struct rtnl_link_stats *stats = ifa->ifa_data;
      bool up, running;

      ifl = xmalloc (sizeof (struct iflist));

      ifl->ifname = xstrdup (ifa->ifa_name);
      ifl->tx_packets = stats->tx_packets;
      ifl->rx_packets = stats->rx_packets;
      ifl->tx_bytes   = stats->tx_bytes; 
      ifl->rx_bytes   = stats->rx_bytes;
      ifl->tx_errors  = stats->tx_errors;
      ifl->rx_errors  = stats->rx_errors;
      ifl->tx_dropped = stats->tx_dropped;
      ifl->rx_dropped = stats->rx_dropped;
      ifl->collisions = stats->collisions;

      if ((check_link (sock, ifa->ifa_name, &up, &running) < 0))
	ifl->link_up = ifl->link_running = -1;
      else
	{
	  ifl->link_up = up;
	  ifl->link_running = running;
	}

      ifl->multicast  = stats->multicast;
      ifl->next = NULL;

      if (iflhead == NULL)
	iflhead = ifl;
      else
	iflprev->next = ifl;
      iflprev = ifl;
    }

  close (sock);
  freeifaddrs (ifaddr);

  return iflhead;
}

struct iflist *
netinfo (unsigned int options, const char *ifname_regex, unsigned int seconds)
{
  bool opt_check_link = (options & CHECK_LINK);
  char msgbuf[256];
  int rc;
  regex_t regex;
  struct iflist *iflhead;

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
      struct iflist *ifl, *ifl2, *iflhead2 =
	get_netinfo_snapshot (options, &regex);

      for (ifl = iflhead, ifl2 = iflhead2; ifl != NULL && ifl2 != NULL;
	   ifl = ifl->next, ifl2 = ifl2->next)
	{
	  if (STRNEQ (ifl->ifname, ifl2->ifname))
	    plugin_error (STATE_UNKNOWN, 0,
			  "bug in netinfo(), please contact the developers");

	  dbg ("network interface '%s'\n", ifl->ifname);

	  dbg ("\ttx_packets : %u %u\n", ifl->tx_packets, ifl2->tx_packets);
	  ifl->tx_packets = (ifl2->tx_packets - ifl->tx_packets) / seconds;

	  dbg ("\trx_packets : %u %u\n", ifl->rx_packets, ifl2->rx_packets);
	  ifl->rx_packets = (ifl2->rx_packets - ifl->rx_packets) / seconds;

	  dbg ("\ttx_bytes   : %u %u\n", ifl->tx_bytes, ifl2->tx_bytes);
	  ifl->tx_bytes   = (ifl2->tx_bytes   - ifl->tx_bytes  ) / seconds;

	  dbg ("\trx_bytes   : %u %u\n", ifl->rx_bytes, ifl2->rx_bytes);
	  ifl->rx_bytes   = (ifl2->rx_bytes   - ifl->rx_bytes  ) / seconds;

	  dbg ("\ttx_errors  : %u %u\n", ifl->tx_errors, ifl2->tx_errors);
	  ifl->tx_errors  = (ifl2->tx_errors  - ifl->tx_errors ) / seconds;

	  dbg ("\trx_errors  : %u %u\n", ifl->rx_errors, ifl2->rx_errors);
	  ifl->rx_errors  = (ifl2->rx_errors  - ifl->rx_errors ) / seconds;

	  ifl->tx_dropped = (ifl2->tx_dropped - ifl->tx_dropped) / seconds;
	  dbg ("\ttx_dropped : %u %u\n", ifl->tx_dropped, ifl2->tx_dropped);

	  dbg ("\trx_dropped : %u %u\n", ifl->rx_dropped, ifl2->rx_dropped);
	  ifl->rx_dropped = (ifl2->rx_dropped - ifl->rx_dropped) / seconds;

	  dbg ("\tcollisions : %u %u\n", ifl->collisions, ifl2->collisions);
	  ifl->collisions = (ifl2->collisions - ifl->collisions) / seconds;

	  dbg ("\tmulticast  : %u %u\n", ifl->multicast, ifl2->multicast);
	  ifl->multicast  = (ifl2->multicast  - ifl->multicast ) / seconds;

	  dbg ("\tlink status: %s %s\n",
	       ifl->link_up < 0 ? "UNKNOWN" : (ifl->link_up ? "UP" : "DOWN"),
	       ifl->link_running < 0 ? "UNKNOWN" :
		 (ifl->link_running ? "RUNNING" : "NOT-RUNNING"));

	  if (opt_check_link && !(ifl->link_up == 1 && ifl->link_running == 1))
	    plugin_error (STATE_CRITICAL, 0,
			  "%s matches the given regular expression "
			  "but is not UP and RUNNING!", ifl->ifname);
	}

      freeiflist (iflhead2);
  }

  /* Free memory allocated to the pattern buffer by regcomp() */
  regfree (&regex);

  return iflhead;
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
