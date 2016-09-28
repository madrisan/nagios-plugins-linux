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
test_clock_zero_timedelta ()
{
  unsigned long refclock;
  long timedelta;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  status = set_thresholds (&my_threshold, "5", "10");
  if (status == NP_RANGE_UNPARSEABLE)
    return EXIT_AM_HARDFAIL;

  refclock = get_current_datetime ();
  timedelta = get_timedelta (refclock, false);

  status = get_status (labs (timedelta), my_threshold);
  free (my_threshold);

  if (status != STATE_OK)
    return -1;

  return 0;
}

static int
mymain (void)
{
  int ret = 0;

  if (test_run
      ("check clock with timedelta set to zero", test_clock_zero_timedelta,
       NULL) < 0)
    ret = -1;

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
