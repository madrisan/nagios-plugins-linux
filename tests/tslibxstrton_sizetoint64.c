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
  char *size;
  int64_t expect_value;
} test_data;

static int
test_sizetoint64 (const void *tdata)
{
  const struct test_data *data = tdata;
  int64_t result;
  char *errmegs;
  int ret = 0;

  sizetoint64 (data->size, &result, &errmegs);
  TEST_ASSERT_EQUAL_NUMERIC (result, data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

# define DO_TEST(SIZE, EXPECT_VALUE)                              \
  do                                                              \
    {                                                             \
      test_data data = {                                          \
        .size = SIZE,                                             \
        .expect_value = EXPECT_VALUE,                             \
      };                                                          \
      if (test_run("check function sizetoint64 with arg " SIZE,   \
                   test_sizetoint64, (&data)) < 0)                \
        ret = -1;                                                 \
    }                                                             \
  while (0)

  /* test the function sizetoint64() */

#define ONE_KILOBYTE 1000UL
#define ONE_MEGABYTE (1000UL * ONE_KILOBYTE)
#define ONE_GIGABYTE (1000UL * ONE_MEGABYTE)
#define ONE_TERABYTE (1000UL * ONE_GIGABYTE)
#define ONE_PETABYTE (1000UL * ONE_TERABYTE)

  DO_TEST ("1024b", 1024);
  DO_TEST ("8k", 8 * ONE_KILOBYTE);
  DO_TEST ("50m", 50 * ONE_MEGABYTE);
  DO_TEST ("2g", 2 * ONE_GIGABYTE);
  DO_TEST ("3t", 3 * ONE_TERABYTE);
  DO_TEST ("2p", 2 * ONE_PETABYTE);

  DO_TEST ("1024B", 1024);
  DO_TEST ("8K", 8 * ONE_KILOBYTE);
  DO_TEST ("50M", 50 * ONE_MEGABYTE);
  DO_TEST ("2G", 2 * ONE_GIGABYTE);
  DO_TEST ("3T", 3 * ONE_TERABYTE);
  DO_TEST ("2P", 2 * ONE_PETABYTE);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
