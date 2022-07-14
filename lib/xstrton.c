// SPDX-License-Identifier: GPL-3.0-or-later
/* strtol with error checking

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"
#include "messages.h"
#include "xasprintf.h"

static int
agemultiplier (double age, char mult, int64_t *result)
{
  double temp = age;

  switch (mult)
    {
    case 0:
    case 's':
    case 'S':
      break;

    case 'm':
    case 'M':
      temp *= 60;
      break;

    case 'h':
    case 'H':
      temp *= 3600;
      break;

    case 'd':
    case 'D':
      temp *= 86400;
      break;

    case 'w':
    case 'W':
      temp *= 7 * 86400;
      break;

    case 'y':
    case 'Y':
      temp *= 31557600; /* == 365.25 * 86400 */
      break;

    default:
      return -1;
    }

  *result = (int64_t) temp;
  return 0;
}

static int
sizemultiplier (double size, char mult, int64_t *result)
{
  double temp = size;

  switch (mult)
    {
    case 0:
    case 'b':
    case 'B':
      break;

    case 'k':
    case 'K':
      temp *= 1000.0;
      break;

    case 'm':
    case 'M':
      temp *= 1000.0 * 1000.0;
      break;

    case 'g':
    case 'G':
      temp *= 1000.0 * 1000.0 * 1000.0;
      break;

    case 't':
    case 'T':
      temp *= 1000.0 * 1000.0 * 1000.0 * 1000.0;
      break;

    case 'p':
    case 'P':
      temp *= 1000.0 * 1000.0 * 1000.0 * 1000.0 * 1000.0;
      break;

    default:
      return -1;
    }

  *result = (int64_t) temp;
  return 0;
}

static int
strtoint64 (const char *str, int64_t *result,
	    int (*converter) (double, char, int64_t *), char **errmesg)
{
  if (str != NULL && *str != '\0')
    {
      char *end = NULL;
      errno = 0;
      double temp = strtod (str, &end);
      if ((errno == 0) && (end != NULL) && (end != str))
	{
	  int res = converter (temp, *end, result);
	  if (res < 0)
	    {
	      *errmesg = xasprintf ("invalid suffix `%c' in `%s'", *end, str);
	      return -1;
	    }

	  if (*(end + 1) != '\0')
	    {
	      *errmesg = xasprintf ("invalid trailing character `%c' in `%s'",
				    *(end + 1), str);
	      return -1;
	    }

	  return 0;
	}

      *errmesg = xasprintf ("converting `%s' to a number failed", str);
      return -1;
    }

  *errmesg = xasprintf ("no number to covert (empty string)");
  return -1;
}

/* convert a string with an optional prefix s (second), m (minute), h (hour),
 * d (day), w (week), or y (year);
 * return 0 on success, -1 otherwise (in this case errmesg will provide a
 * human readable error message)  */
int
agetoint64 (const char *str, int64_t * age, char **errmesg)
{
  return strtoint64 (str, age, agemultiplier, errmesg);
}

/* convert a string with an optional prefix b (byte), k (kilobyte),
 * m (megabyte), g (gigabyte), t (terabyte), or p (petabyte);
 * return 0 on success, -1 otherwise (in this case errmesg will provide a
 * human readable error message)  */
int
sizetoint64 (const char *str, int64_t *size, char **errmesg)
{
  return strtoint64 (str, size, sizemultiplier, errmesg);
}

/* same as strtol(3) but exit on failure instead of returning crap */
long
strtol_or_err (const char *str, const char *errmesg)
{
  char *end = NULL;

  if (str != NULL && *str != '\0')
    {
      errno = 0;
      long num = strtol (str, &end, 10);
      if (errno == 0 && str != end && end != NULL && *end == '\0')
	return num;
    }

  plugin_error (STATE_UNKNOWN, errno, "%s: '%s'", errmesg, str);
  return 0;
}
