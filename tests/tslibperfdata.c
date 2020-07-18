// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2019 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/perfdata.c
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

#include "common.h"
#include "perfdata.h"
#include "system.h"
#include "testutils.h"
#include "thresholds.h"
#include "units.h"

typedef struct test_data
{
  char *warn_string;
  char *critical_string;
  unsigned long base;
  int shift;
  unsigned long long limit;
  bool percent;
  unsigned long long expect_value[2];
} test_data;

static int
test_get_perfdata_limit_converted (const void *tdata)
{
  const struct test_data *data = tdata;
  thresholds *my_threshold = NULL;
  unsigned long long limit_warning, limit_critical;
  int ret = 0, retw, retc, status;

  status = set_thresholds (&my_threshold, data->warn_string,
			   data->critical_string);
  if (status == NP_RANGE_UNPARSEABLE)
    return -1;

  retw =
    get_perfdata_limit_converted (my_threshold->warning, data->base,
				  data->shift, &limit_warning, data->percent);
  retc =
    get_perfdata_limit_converted (my_threshold->critical, data->base,
				  data->shift, &limit_critical,
				  data->percent);
  if (retw != 0 || retc != 0)
    return -1;

  TEST_ASSERT_EQUAL_NUMERIC (limit_warning, data->expect_value[0]);
  retw = ret;
  TEST_ASSERT_EQUAL_NUMERIC (limit_critical, data->expect_value[1]);
  retc = ret;
  if (retw != 0 || retc != 0)
    ret = -1;

  free (my_threshold);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(WARN_STR, CRITICAL_STR, BASE, SHIFT, PERCENT,\
		EXPECT_VALUE_W, EXPECT_VALUE_C)           \
  do                                                      \
    {                                                     \
      test_data data = {                                  \
	.warn_string = WARN_STR,                          \
	.critical_string = CRITICAL_STR,                  \
	.base = BASE,                                     \
	.shift = SHIFT,                                   \
	.percent = PERCENT,                               \
	.expect_value[0] = EXPECT_VALUE_W,                \
	.expect_value[1] = EXPECT_VALUE_C,                \
      };                                                  \
      if (test_run(                                       \
	    "check get_perfdata_limit_converted with -w " \
	    WARN_STR " -c " CRITICAL_STR " and " #SHIFT,  \
                   test_get_perfdata_limit_converted, (&data)) < 0) \
	ret = -1;                                         \
    }                                                     \
  while (0)

  DO_TEST ("20%:", "10%:", 16302692L, k_shift, true, 3260538L, 1630269L);
  DO_TEST ("20%:", "10%:", 16302692L, m_shift, true, 3184L, 1592L);
  DO_TEST ("20%:", "10%:", 16302692L, g_shift, true, 3L, 1L);

  DO_TEST ("80%", "90%", 16302692L, k_shift, true, 13042153L, 14672422L);
  DO_TEST ("80%", "90%", 16302692L, m_shift, true, 12736L, 14328L);
  DO_TEST ("80%", "90%", 16302692L, g_shift, true, 12L, 13L);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
