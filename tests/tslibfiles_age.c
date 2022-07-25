// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/files.c
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
#  include "../lib/files.c"
# undef NPL_TESTING

typedef struct test_data
{
  int64_t age;
  time_t filemtime;
  time_t now;
  bool expect_value;
} test_data;

static int
test_files_check_age (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  bool check = files_check_age (data->age, data->now, data->filemtime);
  TEST_ASSERT_EQUAL_NUMERIC (check, data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

# define DO_TEST(AGE, NOW, FILEMTIME, EXPECT_VALUE)      \
  do                                                     \
    {                                                    \
      test_data data = {                                 \
        .age = AGE,                                      \
        .filemtime = FILEMTIME,                          \
        .now = NOW,                                      \
        .expect_value = EXPECT_VALUE,                    \
      };                                                 \
      if (test_run("check function files_check_age",     \
                   test_files_check_age, (&data)) < 0)   \
        ret = -1;                                        \
    }                                                    \
  while (0)

  /* test the function files_check_age() */

  int64_t age = 3600 * 8; /* 8 hours ago */
  time_t now;

  now = time (NULL);

  DO_TEST (0, now, 0, true);

  DO_TEST (+age, now, now - (age + 1), true);
  DO_TEST (+age, now, now - (age - 1), false);
  DO_TEST (-age, now, now - (age - 1), true);
  DO_TEST (-age, now, now - (age + 1), false);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
