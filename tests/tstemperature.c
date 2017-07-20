/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_temperature.c
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
#include <stdlib.h>

#include "common.h"
#include "testutils.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__ ((unused));
static _Noreturn void usage (FILE * out) __attribute__ ((unused));

#define NPL_TESTING
#include "../plugins/check_temperature.c"

static int
mymain (void)
{
  int chk_temp = 1000*25;
  char *scale;
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC_VERBOSE (
    get_real_temp (chk_temp, &scale, TEMP_CELSIUS), 25);
  TEST_ASSERT_EQUAL_NUMERIC_VERBOSE (
    get_real_temp (chk_temp, &scale, TEMP_FAHRENHEIT), 77);
  TEST_ASSERT_EQUAL_NUMERIC_VERBOSE (
    get_real_temp (chk_temp, &scale, TEMP_KELVIN), 298.1);

  /* FIXME: we should test here /sys/class/thermal/ related stuff
            /sys/class/thermal/thermal_zone[0-*]/{type,temp,trip_point_*}
            see: ./lib/sysfsparser.c  */

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)

#undef NPL_TESTING
