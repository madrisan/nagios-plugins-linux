/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_uptime.c
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

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__((unused));
static _Noreturn void usage (FILE * out) __attribute__((unused));
static double uptime_sysinfo () __attribute__((unused));
#if HAVE_CLOCK_GETTIME_MONOTONIC
static double uptime_clock_monotonic () __attribute__((unused));
#endif

#define NPL_TESTING
# include "../plugins/check_uptime.c"
#undef NPL_TESTING

struct test_data
{
  double secs;
  char *output;
};

static int
test_uptime_output (const void *tdata)
{
  int ret = 0;
  const struct test_data *data = tdata;

  TEST_ASSERT_EQUAL_STRING (sprint_uptime (data->secs), data->output);
  return ret;
}

#define ONE_MINUTE 60
#define ONE_HOUR (60*ONE_MINUTE)
#define ONE_DAY (24*ONE_HOUR)
#define ONE_MONTH (30*ONE_DAY)

static int
mymain (void)
{
  int ret = 0;
  struct test_data data;

#define DO_TEST(SECS, UPTIME_STR)                          \
  do                                                       \
    {                                                      \
      data.secs = SECS;                                    \
      data.output = UPTIME_STR;                            \
      if (test_run ("check uptime ouput of "#SECS" secs",  \
		    test_uptime_output, &data) < 0)        \
	ret = -1;                                          \
    }                                                      \
  while (0)

  DO_TEST (59, "0 min");
  DO_TEST (3*ONE_HOUR+2*ONE_MINUTE, "3 hours 2 min");
  DO_TEST (1*ONE_DAY+1*ONE_HOUR+25*ONE_MINUTE, "1 day 1 hour 25 min");
  DO_TEST (9*ONE_MONTH+4*ONE_HOUR+10*ONE_MINUTE, "270 days 4 hours 10 min");

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#undef ONE_MONTH
#undef ONE_DAY
#undef ONE_HOUR
#undef ONE_MINUTE

TEST_MAIN (mymain);
