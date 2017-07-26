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

#define ARRAY_CARDINALITY(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))

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
  double expect_value[3];
  unsigned int active_cpu;
} test_data;

static int
test_loadavg_normalize (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;
  size_t i;

  normalize_loadavg ((double *) data->loadavg, data->active_cpu);

  for (i = 0; i < ARRAY_CARDINALITY (data->loadavg); ++i)
    TEST_ASSERT_EQUAL_NUMERIC (data->loadavg[i], data->expect_value[i]);

  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(L1, L2, L3, E1, E2, E3, NCPU)                         \
  do                                                                  \
    {                                                                 \
      test_data data = {                                              \
	.loadavg = { L1, L2, L3 },                                    \
	.expect_value = { E1, E2, E3 },                               \
	.active_cpu = NCPU                                            \
      };                                                              \
      if (test_run("check load normalization with " #NCPU " cpu(s)",  \
		   test_loadavg_normalize, (&data)) < 0)              \
	ret = -1;                                                     \
    }                                                                 \
  while (0)

  DO_TEST (/* loadavg: */ 4.0, 2.0, 1.0, /* expect_value: */ 4.0, 2.0, 1.0, 1);
  DO_TEST (/* loadavg: */ 4.0, 2.0, 1.0, /* expect_value: */ 2.0, 1.0, 0.5, 2);
  DO_TEST (/* loadavg: */ 4.0, 2.0, 6.0, /* expect_value: */ 0.5, .25, .75, 8);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
