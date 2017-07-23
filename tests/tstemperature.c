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
#include "sysfsparser.h"
#include "testutils.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__ ((unused));
static _Noreturn void usage (FILE * out) __attribute__ ((unused));

#define NPL_TESTING
#include "../plugins/check_temperature.c"

struct test_data
{
  int unit;
  double shouldbe;
};

static int
test_temperature_unit_conversion (const void *tdata)
{
  char *scale;
  const struct test_data *data = tdata;
  const unsigned long chk_temp = 1000*25;  /* 25C */
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC (data->shouldbe,
			     get_real_temp (chk_temp, &scale, data->unit));
  return ret;
}

static int
mymain (void)
{
  int ret = 0;
  struct test_data data;

  data.unit = TEMP_CELSIUS;
  data.shouldbe = 25;
  TEST_DATA ("get_real_temp with temp_units eq TEMP_CELSIUS",
	     test_temperature_unit_conversion, &data);

  data.unit = TEMP_FAHRENHEIT;
  data.shouldbe = 77;
  TEST_DATA ("get_real_temp with temp_units eq TEMP_FAHRENHEIT",
             test_temperature_unit_conversion, &data);

  data.unit = TEMP_KELVIN;
  data.shouldbe = 298.1;
  TEST_DATA ("get_real_temp with temp_units eq TEMP_KELVIN",
             test_temperature_unit_conversion, &data);

  /* FIXME: we should test here /sys/class/thermal/ related stuff
            /sys/class/thermal/thermal_zone[0-*]/{type,temp,trip_point_*}
            see: ./lib/sysfsparser.c  */

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)

#undef NPL_TESTING
