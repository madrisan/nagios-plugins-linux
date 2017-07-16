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
#include "thresholds.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__ ((unused));
static _Noreturn void usage (FILE * out) __attribute__ ((unused));
static _Noreturn void validate_input (int i, double w, double c)
  __attribute__ ((unused));

#define NPL_TESTING
#include "../plugins/check_load.c"
#undef NPL_TESTING

struct test_data
{
  nagstatus status;
  double loadavg[3];
  double wload[3];
  double cload[3];
  bool required[3];
  bool normalize;
};

static int
test_loadavg_exit_status (const void *tdata)
{
  const struct test_data *data = tdata;
  bool required[] = { true, true, true };
  nagstatus status;

  status = loadavg_status (data->loadavg, data->wload, data->cload, required);
  if (status == data->status)
    return 0;

  return -1;
}

#define TEST_DATA(MSG, FUNC, DATA)         \
  do {                                     \
    if (test_run (MSG, FUNC, DATA) < 0)    \
      ret = -1;                            \
  } while (0)

static int
mymain (void)
{
  int ret = 0;
  double test_loadavg[] = { 4.0, 2.0, 1.0 };

  normalize_loadavg (test_loadavg, 2);
  TEST_ASSERT_EQUAL_NUMERIC ((int) test_loadavg[0], 2);
  TEST_ASSERT_EQUAL_NUMERIC ((int) test_loadavg[1], 1);
  TEST_ASSERT_EQUAL_NUMERIC ((int) (test_loadavg[2] * 2), 1);

  struct test_data tdata_ok = {
    .status = STATE_OK,
    .loadavg = { 2.8, 1.9, 1.3 },
    .wload = { 3.0, 3.0, 3.0 },
    .cload = { 4.0, 4.0, 4.0 }
  };
  TEST_DATA ("check loadavg for ok condition",
	     test_loadavg_exit_status, &tdata_ok);

  struct test_data tdata_warning = {
    .status = STATE_WARNING,
    .loadavg = { 2.8, 1.9, 1.3 },
    .wload = { 3.0, 1.5, 1.5 },
    .cload = { 4.0, 4.0, 4.0 }
  };
  TEST_DATA ("check loadavg for warning condition",
	     test_loadavg_exit_status, &tdata_warning);

  struct test_data tdata_critical = {
    .status = STATE_CRITICAL,
    .loadavg = { 2.8, 1.9, 5.5 },
    .wload = { 1.0, 2.0, 2.0 },
    .cload = { 3.0, 3.0, 4.0 }
  };
  TEST_DATA ("check loadavg for critical condition",
	     test_loadavg_exit_status, &tdata_critical);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#undef TEST_DATA

TEST_MAIN (mymain)
