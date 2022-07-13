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
#include <stdlib.h>

#include "common.h"
#include "messages.h"

/* convert a string with an optional prefix b (byte), k (kilobyte),
 * m (megabyte), g (gigabyte), t (terabyte), or p (petabyte);
 * exit on failure instead of returning crap.   */
long
sizetol (const char *str)
{
  if (str != NULL && *str != '\0')
    {
      char *end = NULL;
      errno = 0;
      double temp = strtod (str, &end);
      if ((errno == 0) && (end != NULL) && (end != str))
	{
	  switch (*end)
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
	      plugin_error (STATE_UNKNOWN, errno,
			    "invalid suffix `%c' in `%s'", *end, str);
	    }

	  return (long)temp;
	}
    }

  plugin_error (STATE_UNKNOWN, errno,
		"converting `%s' to a number failed", str);
  return 0;
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
