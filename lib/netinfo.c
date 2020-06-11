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

#include <assert.h>
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
#include <math.h>
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

static const struct link_speed
{
  long phy_speed;
  const char *phy_speed_str;
} phy_speed_to_str[] = {
  { SPEED_10,      "10Mbps"  },
  { SPEED_100,     "100Mbps" },
  { SPEED_1000,    "1Gbps"   },
  { SPEED_2500,    "2.5Gbps" },
  { SPEED_5000,    "5Gbps"   },
  { SPEED_10000,   "10Gbps"  },
#if defined (SPEED_14000)
  { SPEED_14000,   "14Gbps"  },
#endif
#if defined (SPEED_20000)
  { SPEED_20000,   "20Gbps"  },
#endif
#if defined (SPEED_25000)
  { SPEED_25000,   "25Gbps"  },
#endif
#if defined (SPEED_40000)
  { SPEED_40000,   "40Gbps"  },
#endif
#if defined (SPEED_50000)
  { SPEED_50000,   "50Gbps"  },
#endif
#if defined (SPEED_56000)
  { SPEED_56000,   "56Gbps"  },
#endif
#if defined (SPEED_100000)
  { SPEED_100000,  "100Gbps" },
#endif
  { SPEED_UNKNOWN, "unknown" }
};
static const int link_speed_size =
  sizeof (phy_speed_to_str) / sizeof (phy_speed_to_str[0]);

const char* map_speed_value_for_key (const struct link_speed * map,
				     long phy_speed)
{
  const char* ret = NULL;
  for (size_t i = 0 ; i < link_speed_size && ret == NULL; i++)
    if (map[i].phy_speed == phy_speed)
      ret = map[i].phy_speed_str;

  return ret;
}

static int
check_link_speed (int sock, const char *ifname, unsigned int *speed,
		  int *duplex)
{
  struct ifreq ifr;
  struct ethtool_cmd ecmd;

  ecmd.cmd = ETHTOOL_GSET;
  memset (&ifr, 0, sizeof (ifr));
  STRNCPY_TERMINATED (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
  ifr.ifr_data = (caddr_t) &ecmd;

  dbg ("network interface '%s'\n", ifname);
  if (ioctl (sock, SIOCETHTOOL, &ifr) == 0)
    {
      if (ecmd.supported & SUPPORTED_TP)
	dbg (" twisted pair\n");
      if (ecmd.supported & SUPPORTED_10baseT_Half)
	dbg (" 10Mbit/s\n");
      if (ecmd.supported & SUPPORTED_10baseT_Full)
	dbg (" 10Mbit/s (full duplex)\n");
      if (ecmd.supported & SUPPORTED_100baseT_Half)
	dbg (" 100Mbit/s\n");
      if (ecmd.supported & SUPPORTED_100baseT_Full)
	dbg (" 100Mbit/s (full duplex)\n");
      if (ecmd.supported & SUPPORTED_1000baseT_Half)
	dbg (" 1Gbit/s\n");
      if (ecmd.supported & SUPPORTED_1000baseT_Full)
	dbg (" 1Gbit/s (full duplex)\n");
      if (ecmd.supported & SUPPORTED_10000baseT_Full)
	dbg (" 10Gbit/s (full duplex)\n");
      if (ecmd.supported & SUPPORTED_Autoneg)
	dbg (" auto-negotiation\n");

      const char *speed_str =
	map_speed_value_for_key (phy_speed_to_str, ecmd.speed);
      if (NULL != speed_str)
	{
	  dbg (" speed: %s\n", speed_str);
	  *speed = ecmd.speed;
	}
      else
	{
	  dbg (" speed: unknown/unsupported\n");
	  *speed = 0;
	}

      if (*speed > 0)
	dbg (" max supported speed: %uMbps\n", *speed);

      switch (ecmd.duplex)
	{
	default:
	  dbg (" duplex: unknown\n");
	  *duplex = DUPLEX_UNKNOWN;
	  break;
	case DUPLEX_HALF:
	  dbg (" duplex: half\n");
	  *duplex = DUPLEX_HALF;
	  break;
	case DUPLEX_FULL:
	  dbg (" duplex: full\n");
	  *duplex = DUPLEX_FULL;
	  break;
	}
    }

  return 0;
}

static bool
link_wireless (int sock, const char *ifname)
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

      bool is_wireless = link_wireless (sock, ifa->ifa_name);
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

      check_link_speed (sock, ifa->ifa_name, &(ifl->speed), &(ifl->duplex));

      ifl->multicast = stats->multicast;
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
  struct iflist *iflhead, *ifl, *iflhead2, *ifl2;

  if ((rc =
       regcomp (&regex, ifname_regex ? ifname_regex : ".*", REG_EXTENDED)))
    {
      regerror (rc, &regex, msgbuf, sizeof (msgbuf));
      plugin_error (STATE_UNKNOWN, 0, "could not compile regex: %s", msgbuf);
    }

  dbg ("getting network informations...\n");
  iflhead = get_netinfo_snapshot (options, &regex);

  assert (seconds > 0);
  sleep (seconds);

  dbg ("getting network informations again (after %us)...\n", seconds);
  iflhead2 = get_netinfo_snapshot (options, &regex);

  for (ifl = iflhead, ifl2 = iflhead2; ifl != NULL && ifl2 != NULL;
       ifl = ifl->next, ifl2 = ifl2->next)
    {
      if (STRNEQ (ifl->ifname, ifl2->ifname))
	plugin_error (STATE_UNKNOWN, 0,
		      "bug in netinfo(), please contact the developers");

      dbg ("network interface '%s'\n", ifl->ifname);

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

      dbg ("\tlink status: %s %s\n",
	   ifl->link_up < 0 ? "UNKNOWN" : (ifl->link_up ? "UP" : "DOWN"),
	   ifl->link_running < 0 ? "UNKNOWN" :
	     (ifl->link_running ? "RUNNING" : "NOT-RUNNING"));

      if (ifl->speed > 0)
	dbg ("\tspeed      : %uMbit/s\n", ifl->speed);

      if (opt_check_link && !(ifl->link_up == 1 && ifl->link_running == 1))
	plugin_error (STATE_CRITICAL, 0,
		      "%s matches the given regular expression "
		      "but is not UP and RUNNING!", ifl->ifname);
    }

  freeiflist (iflhead2);

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
