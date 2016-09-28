/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns the number of seconds elapsed between
 * local time and Nagios time.
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

#include <stdlib.h>

#include "common.h"
#include "thresholds.h"

#define NPL_TESTING
#include "testutils.h"
#include "../plugins/check_clock.c"

struct test_data
{
  char *w;
  char *c;
  long delta;
};

static long
get_current_datetime ()
{
  struct tm *tm;
  time_t t;
  char outstr[32];
  char *end = NULL;

  t = time (NULL);
  tm = localtime (&t);
  if (strftime (outstr, sizeof (outstr), "%s", tm) == 0)
    return EXIT_AM_HARDFAIL;

  return strtol (outstr, &end, 10);
}

static int
test_clock_timedelta (const void *tdata)
{
  unsigned long refclock;
  long timedelta;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  const struct test_data *data = tdata;
  long w_threshold = strtol (data->w, NULL, 10),
    c_threshold = strtol (data->c, NULL, 10);

  status = set_thresholds (&my_threshold, data->w, data->c);
  if (status == NP_RANGE_UNPARSEABLE)
    return EXIT_AM_HARDFAIL;

  refclock = data->delta + get_current_datetime ();
  timedelta = get_timedelta (refclock, false);

  status = get_status (labs (timedelta), my_threshold);
  free (my_threshold);

  if (data->delta <= w_threshold && status == STATE_OK)
    return 0;
  if (data->delta <= c_threshold && status == STATE_WARNING)
    return 0;
  if (data->delta > c_threshold && status == STATE_CRITICAL)
    return 0;

  return -1;
}

#define TEST_DATA(MSG, FUNC, DELTA)        \
  data.delta = DELTA;                      \
  if (test_run (MSG, FUNC, &data) < 0)     \
    ret = -1;

static int
mymain (void)
{
  int ret = 0;
  /* set the warning and critical thresholds */
  struct test_data data = { .w = "20", .c = "40" };

  TEST_DATA ("check clock for ok condition", test_clock_timedelta, 0);
  TEST_DATA ("check clock for warning condition", test_clock_timedelta, 10);
  TEST_DATA ("check clock for critical condition", test_clock_timedelta, 50);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
