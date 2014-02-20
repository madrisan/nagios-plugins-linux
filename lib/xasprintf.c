/* asprintf with out-of-memory checking.

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


#include "config.h"

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "error.h"
#include "xasprintf.h"

char *
xasprintf (const char *format, ...)
{
  va_list args;
  char *result;

  va_start (args, format);
  if (vasprintf (&result, format, args) < 0)
    plugin_error (STATE_UNKNOWN, errno, "asprintf has failed");
  va_end (args);

  return result;
}
