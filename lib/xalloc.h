/* xalloc.h -- malloc with out-of-memory checking
 *
 *  Copyright (C) 1990-2000, 2003-2004, 2006-2012 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef XALLOC_H_
# define XALLOC_H_

# ifdef __cplusplus
extern "C" {
# endif

void *xmalloc (size_t s)
      attribute_malloc attribute_alloc_size ((1));
void *xmemdup (void const *p, size_t s)
      attribute_malloc attribute_alloc_size ((2));
char *xstrdup (char const *str)
      attribute_malloc;
void *xnmalloc (size_t n, size_t s)
      attribute_malloc attribute_alloc_size ((1, 2));

#endif /* XALLOC_H_ */
