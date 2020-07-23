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

#include <errno.h>
#include <ifaddrs.h>
#include <string.h>
#include <unistd.h>
#include <linux/ethtool.h>
#ifdef HAVE_LINUX_IF_LINK_H
# include <linux/if_link.h>
#else
# include <linux/rtnetlink.h>
#endif
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "netinfo.h"
#include "system.h"
#include "xalloc.h"

const char *const duplex_table[_DUP_MAX] = {
  [DUPLEX_HALF] = "half",
  [DUPLEX_FULL] = "full"
};

const struct link_speed
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
  { SPEED_UNKNOWN, "unknown!" }
};
static const int link_speed_size =
  sizeof (phy_speed_to_str) / sizeof (phy_speed_to_str[0]);

static int
get_ctl_fd (void)
{
  int fd;
  int s_errno;

  if ((fd = socket (PF_INET, SOCK_DGRAM, 0)) >= 0)
    return fd;
  s_errno = errno;
  if ((fd = socket (PF_PACKET, SOCK_DGRAM, 0)) >= 0)
    return fd;
  if ((fd = socket (PF_INET6, SOCK_DGRAM, 0)) >= 0)
    return fd;

  errno = s_errno;
  return -1;
}

static const char *
map_speed_value_for_key (const struct link_speed * map, long phy_speed)
{
  const char* ret = NULL;
  for (size_t i = 0 ; i < link_speed_size && ret == NULL; i++)
    if (map[i].phy_speed == phy_speed)
      ret = map[i].phy_speed_str;

  return ret ? ret : "unknown!";
}

/* Determines network interface speed from ETHTOOL_GLINKSETTINGS
 * (requires Linux kernel 4.9+).
 * In case of failure revert to obsolete ETHTOOL_GSET. */

static int
check_link_speed (const char *ifname, uint32_t *speed, uint8_t *duplex)
{
  int fd, ret = -1;
  struct ifreq ifr = {};
  struct ethtool_cmd ecmd = {
    .cmd = ETHTOOL_GSET
  };
#ifdef ETHTOOL_GLINKSETTINGS
# define ETHTOOL_LINK_MODE_MASK_MAX_KERNEL_NU32  (SCHAR_MAX)
  struct elinkset {
    struct ethtool_link_settings req;
    uint32_t link_mode_data[3 * ETHTOOL_LINK_MODE_MASK_MAX_KERNEL_NU32];
  } elinkset;
#endif

  if ((fd = get_ctl_fd ()) < 0)
    plugin_error (STATE_UNKNOWN, errno, "socket() failed");

  *duplex = DUPLEX_UNKNOWN;
  *speed = 0;	/* SPEED_UNKNOWN */
  STRNCPY_TERMINATED (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));

#ifdef ETHTOOL_GLINKSETTINGS

  /* The interaction user/kernel via the new API requires a small
   * ETHTOOL_GLINKSETTINGS handshake first to agree on the length
   * of the link mode bitmaps. If kernel doesn't agree with user,
   * it returns the bitmap length it is expecting from user as a
   * negative length (and cmd field is 0). When kernel and user
   * agree, kernel returns valid info in all fields (ie. link mode
   * length > 0 and cmd is ETHTOOL_GLINKSETTINGS). Based on
   * https://github.com/torvalds/linux/commit/3f1ac7a700d039c61d8d8b99f28d605d489a60cf
   */

  dbg ("ETHTOOL_GLINKSETTINGS is defined\n");
  memset (&elinkset, 0, sizeof (elinkset));
  elinkset.req.cmd = ETHTOOL_GLINKSETTINGS;
  ifr.ifr_data = (void *) &elinkset;

  ret = ioctl (fd, SIOCETHTOOL, &ifr);
  /* see above: we expect a strictly negative value from kernel. */
  if (ret >= 0 && (elinkset.req.link_mode_masks_nwords < 0))
    {
      elinkset.req.cmd = ETHTOOL_GLINKSETTINGS;
      elinkset.req.link_mode_masks_nwords = -elinkset.req.link_mode_masks_nwords;

      ret = ioctl (fd, SIOCETHTOOL, &ifr);
      if (ret < 0 || elinkset.req.link_mode_masks_nwords <= 0
	  || elinkset.req.cmd != ETHTOOL_GLINKSETTINGS)
	ret = -ENOTSUP;
    }

#endif

  if (ret < 0)
    {
      dbg ("%s: revert to the obsolete ETHTOOL_GSET...\n", ifname);
      ifr.ifr_data = (void *) &ecmd;
      if ((ret = ioctl (fd, SIOCETHTOOL, &ifr)) == 0)
	{
	  *duplex = ecmd.duplex;
	  *speed = ecmd.speed;
	}
      else
	dbg ("%s: no link speed associated to this interface\n", ifname);
    }
#ifdef ETHTOOL_GLINKSETTINGS
  else
    {
      *duplex = elinkset.req.duplex;
      *speed = elinkset.req.speed;
    }
#endif

  dbg ("%s: duplex %s (%d), speed is %s (%u)\n"
       , ifname
       , *duplex == _DUP_UNKNOWN ? "invalid" : duplex_table[*duplex]
       , *duplex
       , map_speed_value_for_key (phy_speed_to_str, *speed)
       , *speed);

  /* normalize the unknown speed values */
  if (*speed == (uint16_t)(-1) || *speed == (uint32_t)(-1))
    {
      dbg ("%s: normalizing the unknown speed to zero...\n", ifname);
      *speed = 0;
    }

  close (fd);
  return ret;
}

static bool
link_wireless (const char *ifname)
{
  bool is_wireless = false;
  int sock;
  struct iwreq pwrq;

  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    plugin_error (STATE_UNKNOWN, errno, "socket() failed");

  memset (&pwrq, 0, sizeof (pwrq));
  STRNCPY_TERMINATED (pwrq.ifr_name, ifname, IFNAMSIZ);

  if (ioctl (sock, SIOCGIWNAME, &pwrq) != -1)
    {
      dbg ("%s: wireless interface (%s)\n"
	   , ifname
	   , pwrq.u.name);
      is_wireless = true;
    }

  close (sock);
  return is_wireless;
}

struct iflist *
get_netinfo_snapshot (unsigned int options, const regex_t *iface_regex)
{
  bool opt_ignore_loopback = (options & NO_LOOPBACK),
       opt_ignore_wireless = (options & NO_WIRELESS);
  int family, sock;
  struct ifaddrs *ifa, *ifaddr;
  struct iflist *iflhead = NULL, *iflprev = NULL, *ifl;

  if ((sock = get_ctl_fd ()) < 0)
    plugin_error (STATE_UNKNOWN, errno, "socket() failed");

  if (getifaddrs (&ifaddr) == -1)
    plugin_error (STATE_UNKNOWN, errno, "getifaddrs() failed");

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL)
	continue;

      family = ifa->ifa_addr->sa_family;

      dbg ("%s: address family %d%s\n", ifa->ifa_name, family,
	   (family == AF_PACKET) ? " (AF_PACKET)" :
	   (family == AF_INET)   ? " (AF_INET) ... skip"   :
	   (family == AF_INET6)  ? " (AF_INET6) ... skip"  : "");
      if (family == AF_INET || family == AF_INET6)
	continue;

      bool is_loopback = if_flags_LOOPBACK (ifa->ifa_flags);
      bool is_wireless = link_wireless (ifa->ifa_name);
      bool skip_interface =
	((is_loopback && opt_ignore_loopback)
	 || (is_wireless && opt_ignore_wireless)
	 || (regexec (iface_regex, ifa->ifa_name, (size_t) 0, NULL, 0)));
      if (skip_interface)
	{
	  dbg ("skipping network interface '%s'...\n", ifa->ifa_name);
	  continue;
	}

      if (family == AF_PACKET && ifa->ifa_data != NULL)
	{
	  struct rtnl_link_stats *stats = ifa->ifa_data;

	  ifl = xmalloc (sizeof (struct iflist));
	  ifl->flags      = ifa->ifa_flags;
	  ifl->ifname     = xstrdup (ifa->ifa_name);
	  ifl->tx_packets = stats->tx_packets;
	  ifl->rx_packets = stats->rx_packets;
	  ifl->tx_bytes   = stats->tx_bytes;
	  ifl->rx_bytes   = stats->rx_bytes;
	  ifl->tx_errors  = stats->tx_errors;
	  ifl->rx_errors  = stats->rx_errors;
	  ifl->tx_dropped = stats->tx_dropped;
	  ifl->rx_dropped = stats->rx_dropped;
	  ifl->collisions = stats->collisions;
	  ifl->multicast  = stats->multicast;
	}
      else
	{
	  dbg ("skipping unknown interface %s\n", ifa->ifa_name);
	  continue;
	}

      check_link_speed (ifa->ifa_name, &(ifl->speed), &(ifl->duplex));

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
