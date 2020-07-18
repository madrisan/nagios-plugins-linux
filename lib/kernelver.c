// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the version of a running Linux kernel
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <linux/version.h>

#include "messages.h"

unsigned int
linux_version (void)
{
  struct utsname utsbuf;
  int x, y, z, depth;

  if (uname (&utsbuf) == -1)
    plugin_error (STATE_UNKNOWN, errno, "uname() failed");

  x = y = z = 0;
  depth = sscanf(utsbuf.release, "%d.%d.%d", &x, &y, &z);
  if ((depth < 2) ||	/* non-standard for all known kernels */
       ((depth < 3) && (x < 3)))	/* non-standard for 2.x.x kernels */
    plugin_error (STATE_UNKNOWN, 0, "non-standard kernel version: %s",
		  utsbuf.release);

  return KERNEL_VERSION (x, y, z);
}
