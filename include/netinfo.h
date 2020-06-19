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

#define CHECK_LINK	(1 << 0)
#define NO_LOOPBACK	(1 << 1)
#define NO_WIRELESS	(1 << 2)

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
    int link_up;
    int link_running;
    unsigned int multicast;
    __u32 speed;	/* the link speed in Mbps */
    __u8 duplex;	/* the duplex as defined in <linux/ethtool.h> */
    struct iflist *next;
  } iflist_t;

  struct iflist *netinfo (unsigned int options, const char *ifname_regex,
			  unsigned int seconds, unsigned int *ninterfaces);
  void freeiflist (struct iflist *iflhead);

#ifdef __cplusplus
}
#endif

#endif				/* _NETINFO_H */
