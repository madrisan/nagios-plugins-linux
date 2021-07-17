// SPDX-License-Identifier: GPL-3.0-or-later
/* string-macros.h -- a collection of macros for checking strings

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

#ifndef _STRING_MACROS_H
# define _STRING_MACROS_H     1

#include "system.h"

# define STRCONTAINS(a, b) (NULL != strpbrk(a, b))
# define STREQ(a, b) (strcmp(a, b) == 0)
# define STRNEQ(a, b) (strcmp(a, b) != 0)
# define STREQLEN(a, b, n) (strncmp(a, b, n) == 0)
# define STRNEQLEN(a, b, n) (strncmp(a, b, n) != 0)
# define STRPREFIX(a, b) (strncmp(a, b, strlen(b)) == 0)

# define STRTRIMPREFIX(a, b) STRPREFIX(a, b) ? a + strlen(b) : a

/* Use strncpy with N-1 and ensure the string is terminated.  */
#define STRNCPY_TERMINATED(DEST, SRC, N) \
  do { \
    strncpy (DEST, SRC, N - 1); \
    DEST[N - 1] = '\0'; \
  } while (false)

#endif			/* _STRING_MACROS_H */
