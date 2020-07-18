// SPDX-License-Identifier: GPL-3.0-or-later
/* xalloc.h -- malloc with out-of-memory checking

   Copyright (C) 1990-2000, 2003-2004, 2006-2012 Free Software Foundation, Inc.

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

#ifndef XALLOC_H_
# define XALLOC_H_

# ifdef __cplusplus
extern "C" {
# endif

  void *xmalloc (const size_t s)
	_attribute_malloc_ _attribute_alloc_size_ ((1));
  void *xmemdup (void const *p, const size_t s)
	_attribute_malloc_ _attribute_alloc_size_ ((2));
  char *xstrdup (char const *str)
	_attribute_malloc_;
  char *xsubstrdup (char const *start, const size_t s)
	_attribute_malloc_;
  void *xnmalloc (const size_t n, const size_t s)
	_attribute_malloc_ _attribute_alloc_size_ ((1, 2));
  void *xrealloc (void *p, const size_t s)
	_attribute_malloc_ _attribute_alloc_size_ ((2));

#ifdef __cplusplus
}
#endif

#endif /* XALLOC_H_ */
