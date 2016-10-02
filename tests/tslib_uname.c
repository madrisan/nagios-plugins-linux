/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns the number of seconds elapsed between
 * local time and Nagios time.
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

#include <stdio.h>
#include <string.h>
#include <linux/version.h>
#include <sys/utsname.h>

#include "common.h"
#include "kernelver.h"
#include "testutils.h"

int
uname (struct utsname *__name)
{
  const char krelease[] = TEST_KERNEL_VERSION;
  strncpy (__name->release, krelease, _UTSNAME_RELEASE_LENGTH);
  return 0;
}
