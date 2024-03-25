// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/xstrton.c
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

#include "testutils.h"

# define NPL_TESTING
#  include "../lib/xstrton.c"
# undef NPL_TESTING

typedef struct test_data
{
  char *age;
  int64_t expect_value;
} test_data;

static int
test_agetoint64 (const void *tdata)
{
  const struct test_data *data = tdata;
  long long int result;
  char *errmegs;
  int ret = 0;

  agetollint (data->age, &result, &errmegs);
  TEST_ASSERT_EQUAL_NUMERIC (result, data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

# define DO_TEST(AGE, EXPECT_VALUE)                             \
  do                                                            \
    {                                                           \
      test_data data = {                                        \
        .age = AGE,                                             \
        .expect_value = EXPECT_VALUE,                           \
      };                                                        \
      if (test_run("check function agetollint with arg " AGE,   \
                   test_agetoint64, (&data)) < 0)               \
        ret = -1;                                               \
    }                                                           \
  while (0)

  /* test the function agetoint64() */

#define ONE_MIN 60
#define ONE_HOUR (60 * ONE_MIN)
#define ONE_DAY (24 * ONE_HOUR)
#define ONE_WEEK (7 * ONE_DAY)
#define ONE_YEAR (365.25 * ONE_DAY)

  DO_TEST ("100s", 100);
  DO_TEST ("30m", 30 * ONE_MIN);
  DO_TEST ("4h", 4 * ONE_HOUR);
  DO_TEST ("10d", 10 * ONE_DAY);
  DO_TEST ("0.5d", 12 * ONE_HOUR);
  DO_TEST ("4w", 4 * ONE_WEEK);
  DO_TEST ("1y", ONE_YEAR);

  DO_TEST ("100S", 100);
  DO_TEST ("30M", 30 * ONE_MIN);
  DO_TEST ("4H", 4 * ONE_HOUR);
  DO_TEST ("10D", 10 * ONE_DAY);
  DO_TEST ("0.5D", 12 * ONE_HOUR);
  DO_TEST ("4W", 4 * ONE_WEEK);
  DO_TEST ("1Y", ONE_YEAR);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
TEST_MAIN (mymain)
