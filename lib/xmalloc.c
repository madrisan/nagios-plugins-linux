/* xmalloc.c -- malloc with out of memory checking
 *
 * Copyright (C) 1990-2000, 2002-2006, 2008-2012 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# include "messages.h"
# include "xalloc.h"

/* Allocate N bytes of memory dynamically, with error checking.
 * The memory is set to zero.  */

void *
xmalloc (const size_t n)
{
  void *p = calloc (1, n);
  if (!p && n != 0)
    plugin_error (STATE_UNKNOWN, errno, "memory exhausted");
  return p;
}

/* Clone an object P of size S, with error checking.  There's no need
 * for xnmemdup (P, N, S), since xmemdup (P, N * S) works without any
 * need for an arithmetic overflow check.  */

void *
xmemdup (void const *p, const size_t s)
{
  return memcpy (xmalloc (s), p, s);
}

/* Clone STRING.  */

char *
xstrdup (char const *string)
{
  return xmemdup (string, strlen (string) + 1);
}

/* Clone a substring of lenght S, starting at the address START.
 * S characters will be copied into the new allocated memory and
 * a null character will be added at the and.  */

char *
xsubstrdup (char const *start, const size_t s)
{
  return memcpy (xmalloc (s + 1), start, s);
}

/* Allocate an array of N objects, each with S bytes of memory,
 *    dynamically, with error checking.  S must be nonzero.  */

void *
xnmalloc (const size_t n, const size_t s)
{
  return xmalloc (n * s);
}

/* Memory allocation wrapper for realloc */

void *
xrealloc (void *ptr, const size_t size)
{
  void *ret = realloc (ptr, size);
  if (!ret && size)
    plugin_error (STATE_UNKNOWN, errno, "memory exhausted");
  return ret;
}
