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

#include <sys/types.h>
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

  enum
  {
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
    unsigned int collisions;
    unsigned int flags;
    unsigned int multicast;
    __u32 speed;	/* the link speed in Mbps */
    __u8 duplex;	/* the duplex as defined in <linux/ethtool.h> */
    struct iflist *next;
  } iflist_t;

  struct iflist *netinfo (unsigned int options, const char *ifname_regex,
			  unsigned int seconds, unsigned int *ninterfaces);
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
