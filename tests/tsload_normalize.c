/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_load.c
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
#include "system.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__ ((unused));
static _Noreturn void usage (FILE * out) __attribute__ ((unused));
static _Noreturn void validate_input (int i, double w, double c)
  __attribute__ ((unused));
static int loadavg_status (const double *loadavg, const double *wload,
			   const double *cload, const bool *required)
  __attribute__ ((unused));

#define NPL_TESTING
#include "../plugins/check_load.c"
#undef NPL_TESTING

typedef struct test_data
{
  double loadavg[3];
} test_data;

static int
test_loadavg_normalize (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  normalize_loadavg ((double *) data->loadavg, 2);

  TEST_ASSERT_EQUAL_NUMERIC (data->loadavg[0], 2);
  TEST_ASSERT_EQUAL_NUMERIC (data->loadavg[1], 1);
  TEST_ASSERT_EQUAL_NUMERIC (2*data->loadavg[2], 1);

  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(MSG, FUNC, DATA) \
  do { if (test_run (MSG, FUNC, DATA) < 0) ret = -1; } while (0)

  test_data tdata_normalize = {
    .loadavg = { 4.0, 2.0, 1.0 }
  };
  DO_TEST ("check normalize_loadavg", test_loadavg_normalize,
	   &tdata_normalize);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
