// SPDX-License-Identifier: GPL-3.0-or-later
/* netinfo.h -- a library foe getting network interfaces statistics

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

#ifndef _NETINFO_H
#define _NETINFO_H

#include <stdint.h>
#include <sys/types.h>

#include "netinfo-private.h"
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

  enum
  {
    /* list of address families */
    IF_AF_PACKET	= (1 << 0),
    IF_AF_INET		= (1 << 1),
    IF_AF_INET6		= (1 << 2),
    /* command-line options */
    CHECK_LINK		= (1 << 0),
    NO_LOOPBACK		= (1 << 1),
    NO_WIRELESS		= (1 << 2),
    NO_BYTES		= (1 << 3),
    NO_COLLISIONS	= (1 << 4),
    NO_DROPS		= (1 << 4),
    NO_ERRORS		= (1 << 5),
    NO_MULTICAST	= (1 << 6),
    NO_PACKETS		= (1 << 7),
    RX_ONLY		= (1 << 8),
    TX_ONLY		= (1 << 9)
  };

  enum duplex
  {
    DUP_HALF = DUPLEX_HALF,
    DUP_FULL = DUPLEX_FULL,
    _DUP_MAX,
    _DUP_UNKNOWN = DUPLEX_UNKNOWN
  };

  struct iflist *netinfo (unsigned int options, const char *ifname_regex,
			  unsigned int seconds, unsigned int *ninterfaces);
  struct iflist *iflist_get_next (struct iflist *ifentry);
#define iflist_foreach(list_entry, list) \
	for (list_entry = list; list_entry != NULL; \
	     list_entry = iflist_get_next(list_entry))

  /* Accessing the values from struct iflist */
  const char *iflist_get_ifname (struct iflist *ifentry);
  uint8_t iflist_get_addr_family (struct iflist *ifentry);
  uint8_t iflist_get_duplex (struct iflist *ifentry);
  uint32_t iflist_get_speed (struct iflist *ifentry);
  unsigned int iflist_get_tx_packets (struct iflist *ifentry);
  unsigned int iflist_get_rx_packets (struct iflist *ifentry);
  unsigned int iflist_get_tx_bytes (struct iflist *ifentry);
  unsigned int iflist_get_rx_bytes (struct iflist *ifentry);
  unsigned int iflist_get_tx_errors (struct iflist *ifentry);
  unsigned int iflist_get_rx_errors (struct iflist *ifentry);
  unsigned int iflist_get_tx_dropped (struct iflist *ifentry);
  unsigned int iflist_get_rx_dropped (struct iflist *ifentry);
  unsigned int iflist_get_collisions (struct iflist *ifentry);
  unsigned int iflist_get_flags (struct iflist *ifentry);
  unsigned int iflist_get_multicast (struct iflist *ifentry);

  void print_ifname_debug (struct iflist *iflhead, unsigned int options);
  void freeiflist (struct iflist *iflhead);

  /* helper functions for parsing and priting interface flags */
  bool if_flags_LOOPBACK (unsigned int flags);
  bool if_flags_RUNNING (unsigned int flags);
  bool if_flags_UP (unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif				/* _NETINFO_H */
