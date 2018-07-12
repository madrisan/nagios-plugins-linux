/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/meminfo.c
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

#include "meminfo.h"
#include "testutils.h"
#include "units.h"

typedef struct test_data
{
  unsigned long long memsize;
  unsigned long long expect_value;
  unit_shift shift;
} test_data;

static int
test_memory_unit_conversion (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC (
    UNIT_CONVERT (data->memsize, data->shift), data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define KILO 1024UL
#define MEGA KILO*KILO

#define DO_TEST(MSG, MEMSIZE, SHIFT, EXPECT_VALUE)                \
  do                                                              \
    {                                                             \
      test_data data = {                                          \
	.memsize = MEMSIZE,                                       \
	.shift = SHIFT,                                           \
	.expect_value = EXPECT_VALUE,                             \
      };                                                          \
      if (test_run(MSG, test_memory_unit_conversion, &data) < 0)  \
        ret = -1;                                                 \
    }                                                             \
  while (0)

  /* unit conversion */
  DO_TEST ("check memory size conversion into kbytes",
	   KILO, k_shift, KILO);
  DO_TEST ("check memory size conversion into mbytes",
	   2*KILO, m_shift, 2UL);
  DO_TEST ("check memory size conversion into gbytes",
	   4*MEGA, g_shift, 4UL);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
