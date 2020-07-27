// SPDX-License-Identifier: GPL-3.0-or-later
/* netinfo-private.h -- a library foe getting network interfaces statistics

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _NETINFO_PRIVATE_H
#define _NETINFO_PRIVATE_H

#include <regex.h>
#include "netinfo.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct ifstats
  {
    unsigned int tx_packets;
    unsigned int rx_packets;
    unsigned int tx_bytes;
    unsigned int rx_bytes;
    unsigned int tx_errors;
    unsigned int rx_errors;
    unsigned int tx_dropped;
    unsigned int rx_dropped;
    unsigned int collisions;
    unsigned int multicast;
  } ifstats_t;

  typedef struct iflist
  {
    char *ifname;
    uint8_t duplex;	   /* the duplex as defined in <linux/ethtool.h> */
    uint32_t speed;	   /* the link speed in Mbps */
    unsigned int flags;
    struct ifstats *stats;
    struct iflist *next;
  } iflist_t;

  struct iflist *get_netinfo_snapshot (unsigned int options,
				       const regex_t *iface_regex);

#ifdef __cplusplus
}
#endif

#endif				/* _NETINFO_PRIVATE_H */
