/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <linux/if_link.h>

#include "common.h"
#include "messages.h"
#include "xalloc.h"

typedef struct iflist
{
  char *ifname;
  unsigned int tx_packets;
  unsigned int rx_packets;
  unsigned int tx_bytes;
  unsigned int rx_bytes;
  unsigned int tx_errors;
  unsigned int rx_errors;
  unsigned int tx_dropped;
  unsigned int rx_dropped;
  unsigned int multicast;
  unsigned int collisions;
  struct iflist *next;
} iflist_t;

struct iflist* netinfo (void)
{
  int family;
  struct ifaddrs *ifaddr, *ifa;

  if (getifaddrs (&ifaddr) == -1)
    plugin_error (STATE_UNKNOWN, errno, "getifaddrs() failed");

  iflist_t *iflhead = NULL, *iflprev = NULL, *ifl;

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_data == NULL)
        continue;

      family = ifa->ifa_addr->sa_family;
      if (family != AF_PACKET)
	continue;

      /* lo-rxkB/s=0.00;;;; lo-txkB/s=0.00;;;; lo-rxerr/s=0.00;;;;
       *  lo-txerr/s=0.00;;;; lo-rxdrop/s=0.00;;;; lo-txdrop/s=0.00;;;;
       * eth1-rxkB/s=231.39;;;; eth1-txkB/s=196.18;;;; eth1-rxerr/s=0.00;;;; 
       *  eth1-txerr/s=0.00;;;; eth1-rxdrop/s=0.00;;;; eth1-txdrop/s=0.00;;;;
       */
      struct rtnl_link_stats *stats = ifa->ifa_data;
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
      ifl->multicast  = stats->multicast;
      ifl->collisions = stats->collisions;
      ifl->next = NULL;

      if (iflhead == NULL)
	iflhead = ifl;
      else
	iflprev->next = ifl;
      iflprev = ifl;
    }

  freeifaddrs (ifaddr);

  return iflhead;
}

void freeiflist (struct iflist *iflhead)
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
