// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test library for checking lib/kernelver.c
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

#include <string.h>
#include <sys/utsname.h>

#include "testutils.h"

/*
 * SUSv3 specifies uname(), but leaves the lengths of the various fields
 * of the utsname structure undefined, requiring only that the strings be
 * terminated by a null byte.  On Linux, these fields are each 65 bytes
 * long; including space for terminating null byte.
 * Glibc declares a variable _UTSNAME_[A-Z]*_LENGTH for each field, all
 * equal to _UTSNAME_LENGTH which in turn equals to 65.
 * Musl libc (shipped by Linux Alpine) does not declare any length variable
 * but hardcodes this value in each field definition.
 */
#if !defined _UTSNAME_RELEASE_LENGTH && defined LIBC_MUSL
# define _UTSNAME_RELEASE_LENGTH 65
#endif

int
uname (struct utsname *__name)
{
  const char krelease[] = TEST_KERNEL_VERSION;
  strncpy (__name->release, krelease, _UTSNAME_RELEASE_LENGTH);
  return 0;
}
