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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# include "xalloc.h"

/* Allocate N bytes of memory dynamically, with error checking.  */

void *
xmalloc (size_t n)
{
  void *p = malloc (n);
  if (!p && n != 0)
    perror ("memory exhausted");
  return p;
}

/* Clone an object P of size S, with error checking.  There's no need
 *    for xnmemdup (P, N, S), since xmemdup (P, N * S) works without any
 *       need for an arithmetic overflow check.  */

void *
xmemdup (void const *p, size_t s)
{
  return memcpy (xmalloc (s), p, s);
}

/* Clone STRING.  */

char *
xstrdup (char const *string)
{
  return xmemdup (string, strlen (string) + 1);
}

/* Allocate an array of N objects, each with S bytes of memory,
 *    dynamically, with error checking.  S must be nonzero.  */

void *
xnmalloc (size_t n, size_t s)
{
  return xmalloc (n * s);
}
