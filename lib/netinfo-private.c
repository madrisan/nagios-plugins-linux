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
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "common.h"
#include "logging.h"
#include "string-macros.h"
#include "messages.h"
#include "netinfo.h"
#include "system.h"
#include "xalloc.h"

#define IFLIST_REPLY_BUFFER	8192

typedef struct nl_req_s
{
  struct nlmsghdr hdr;
  struct rtgenmsg gen;
} nl_req_t;

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
  strncpy (pwrq.ifr_name, ifname, IFNAMSIZ);

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
  char reply[IFLIST_REPLY_BUFFER];
  int fd;
  struct iflist *iflhead = NULL, *iflprev = NULL;
  struct iovec iov;             /* IO vector for sendmsg */
  struct msghdr rtnl_msg;       /* generic msghdr struct for use with sendmsg */
  struct sockaddr_nl kernel;    /* the remote (kernel space) side of the communication */
  union
  {
    struct sockaddr addr;
    struct sockaddr_nl local;   /* local (user space) addr struct */
  } u;
  nl_req_t req;                 /* structure that describes the rtnetlink packet itself */
  pid_t pid = getpid ();

  fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (fd < 0)
    plugin_error (STATE_UNKNOWN, errno, "failed to create netlink socket");

  memset (&u.local, 0, sizeof (u.local));
  u.local.nl_family = AF_NETLINK;
  u.local.nl_groups = 0;
  u.local.nl_pid = pid;

  if (bind (fd, &u.addr, sizeof (u.addr)) < 0)
    {
      close (fd);
      plugin_error (STATE_UNKNOWN, errno, "failed to bind netlink socket");
    }

  /* RTNL socket is ready for use, prepare and send request */

  int msg_done = 0;		/* some flag to end loop parsing */

  memset (&rtnl_msg, 0, sizeof (rtnl_msg));
  memset (&kernel, 0, sizeof (kernel));
  memset (&req, 0, sizeof (req));

  kernel.nl_family = AF_NETLINK;

  req.hdr.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtgenmsg));
  req.hdr.nlmsg_type = RTM_GETLINK;
  req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  req.hdr.nlmsg_seq = 1;
  req.hdr.nlmsg_pid = pid;
  /* no preferred AF, we will get all interfaces */
  req.gen.rtgen_family = AF_PACKET;

  iov.iov_base = &req;
  iov.iov_len = req.hdr.nlmsg_len;
  rtnl_msg.msg_iov = &iov;
  rtnl_msg.msg_iovlen = 1;
  rtnl_msg.msg_name = &kernel;
  rtnl_msg.msg_namelen = sizeof (kernel);

  if (sendmsg (fd, (struct msghdr *) &rtnl_msg, 0) < 0)
    plugin_error (STATE_UNKNOWN, errno, "error in sendmsg");

  while (!msg_done)
    {
      /* parse reply */
      int len;
      struct nlmsghdr *h;
      struct msghdr rtnl_reply;
      struct iovec io_reply;

      memset (&io_reply, 0, sizeof (io_reply));
      memset (&rtnl_reply, 0, sizeof (rtnl_reply));

      iov.iov_base = reply;
      iov.iov_len = IFLIST_REPLY_BUFFER;
      rtnl_reply.msg_iov = &iov;
      rtnl_reply.msg_iovlen = 1;
      rtnl_reply.msg_name = &kernel;
      rtnl_reply.msg_namelen = sizeof (kernel);

      /* read as much data as fits in the receive buffer */
      if ((len = recvmsg (fd, &rtnl_reply, 0)) != 0)
        {
	  bool skip_interface;
	  char name[IFNAMSIZ];
	  int attr_len;
	  struct iflist *ifl;
	  struct ifinfomsg *ifi;
	  struct rtattr *attribute;
	  struct rtnl_link_stats *stats;
	  unsigned int flags = 0;

	  for (h = (struct nlmsghdr *) reply;
	       NLMSG_OK (h, len); h = NLMSG_NEXT (h, len))
	    {
	      switch (h->nlmsg_type)
		{
		default:
		  break;
		/* this is the special meaning NLMSG_DONE message we asked for
		 * by using NLM_F_DUMP flag  */
		case NLMSG_DONE:
		  msg_done = 1;
		  break;
		case RTM_NEWLINK:
		  ifi = NLMSG_DATA (h);
		  attr_len = h->nlmsg_len - NLMSG_LENGTH (sizeof (*ifi));

		  ifl = NULL;
		  flags = ifi->ifi_flags;
		  skip_interface = false;

		  /* loop over all attributes for the NEWLINK message */
		  for (attribute = IFLA_RTA (ifi); RTA_OK (attribute, attr_len);
		       attribute = RTA_NEXT (attribute, attr_len))
		    {
		      switch (attribute->rta_type)
			{
			  case IFLA_IFNAME:
			    strcpy (name, (char *) RTA_DATA (attribute));
			    bool is_loopback = if_flags_LOOPBACK (flags);
			    bool is_wireless = link_wireless (name);
			    skip_interface =
			      ((is_loopback && opt_ignore_loopback)
			       || (is_wireless && opt_ignore_wireless)
			       || (regexec (iface_regex, name, (size_t) 0, NULL, 0)));
			    if (skip_interface)
			      dbg ("skipping network interface '%s'...\n", name);
			    else
			      {
				ifl = xmalloc (sizeof (struct iflist));
				ifl->ifname = xstrdup (name);
				ifl->flags = flags;
				check_link_speed (name, &(ifl->speed), &(ifl->duplex));

				ifl->next = NULL;
				ifl->stats = NULL;

			        if (NULL == iflhead)
				  iflhead = ifl;
				else
				  iflprev->next = ifl;
				iflprev = ifl;
			      }
			    break;
			  case IFLA_STATS:
			    if (ifl)
			      {
				stats = (struct rtnl_link_stats *) RTA_DATA (attribute);
				ifl->stats = xmalloc (sizeof (struct ifstats));
				ifl->stats->collisions = stats->collisions;
				ifl->stats->multicast  = stats->multicast;
				ifl->stats->tx_packets = stats->tx_packets;
				ifl->stats->rx_packets = stats->rx_packets;
				ifl->stats->tx_bytes   = stats->tx_bytes;
				ifl->stats->rx_bytes   = stats->rx_bytes;
				ifl->stats->tx_errors  = stats->tx_errors;
				ifl->stats->rx_errors  = stats->rx_errors;
				ifl->stats->tx_dropped = stats->tx_dropped;
				ifl->stats->rx_dropped = stats->rx_dropped;
			      }
			    else
			      dbg ("skipping network interface stats for '%s'...\n", name);
			    break;
			}
		    }
		  break;
		}
	    }
	}
      usleep (250000);		/* sleep for a while */
    }

  close (fd);
  return iflhead;
}
