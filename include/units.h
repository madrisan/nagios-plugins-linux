// SPDX-License-Identifier: GPL-3.0-or-later
/* units.h -- a library for converting between memory units

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

#ifndef _UNITS_H_
#define _UNITS_H_

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum unit_shift
  {
    b_shift = 0, k_shift = 10, m_shift = 20, g_shift = 30
  } unit_shift;

#define UNIT_CONVERT(X, shift) (((unsigned long long)(X) << k_shift) >> shift)
#define UNIT_STR(X) UNIT_CONVERT(X, shift), units

#ifdef __cplusplus
}
#endif

#endif				/* _UNITS_H_ */
