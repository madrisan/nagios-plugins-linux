/*
 * License: GPLv3+
 * Copyright (c) 2019 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library to manage the Nagios perfdata.
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
 * This software takes some ideas and code from procps-3.2.8 (free).
 */

#include "common.h"
#include "logging.h"
#include "system.h"
#include "thresholds.h"
#include "units.h"

int
get_perfdata_limit (range * threshold, unsigned long base,
		    unsigned long long *limit, bool percent)
{
  if (NULL == threshold)
    return -1;

  double threshold_limit;

  if (NP_RANGE_INSIDE == threshold->alert_on)
    {
      /* example: @10:20  >= 10 and <= 20, (inside the range of {10 .. 20}) */
      dbg ("threshold {%g, %g} with alert_on set to true\n",
	   threshold->start, threshold->end);
      return -1;
    }
  else if (threshold->start_infinity)
    {
      /* example: ~:10    > 10, (outside the range of {-\infty .. 10}) */
      dbg ("threshold {%g, %g} with start_infinity set to true\n",
	   threshold->start, threshold->end);
      return -1;
    }
  else if (threshold->end_infinity)
    {
      /* example: 10:     < 10, (outside {10 .. \infty}) */
      dbg ("threshold {%g, %g} with end_infinity set to true\n",
	   threshold->start, threshold->end);
      threshold_limit = threshold->start;
    }
  else
    {
      /* example: 10      < 0 or > 10, (outside the range of {0 .. 10})
       *          ~:10    > 10, (outside the range of {-\infty .. 10}) */
      dbg ("threshold {%g, %g}\n",
	   threshold->start, threshold->end);
      threshold_limit = threshold->end;
  }

  *limit = (unsigned long long) (base * threshold_limit);
  if (percent)
    *limit = (unsigned long long) (*limit / 100.0);

  return 0;
}

int
get_perfdata_limit_converted (range * threshold, unsigned long base, int shift,
			      unsigned long long *limit, bool percent)
{
  int error = get_perfdata_limit (threshold, base, limit, percent);
  if (0 != error)
    return error;

  *limit = UNIT_CONVERT (*limit, shift);
  return 0;

}
