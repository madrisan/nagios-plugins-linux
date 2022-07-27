// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/kernelver.c
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

#include <linux/version.h>

#include "common.h"
#include "kernelver.h"
#include "testutils.h"

static int
mymain (void)
{
#ifdef KERNEL_VERSION
  int ret = 0;
  unsigned int lnxver = linux_version ();

  TEST_ASSERT_EQUAL_NUMERIC (lnxver,
			     KERNEL_VERSION (TEST_KERNEL_VERSION_MAJOR,
					     TEST_KERNEL_VERSION_MINOR,
					     TEST_KERNEL_VERSION_PATCH));
  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
#else
  return EXIT_AM_SKIP;
#endif
}

TEST_MAIN_PRELOAD (mymain, abs_builddir "/.libs/tslibuname.so");
