// SPDX-License-Identifier: GPL-3.0-or-later
/* perfdata.h -- a library for managing the Nagios perfdata

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

#ifndef _PERFDATA_H_
#define _PERFDATA_H_

#include "system.h"
#include "thresholds.h"

#ifdef __cplusplus
extern "C"
{
#endif

  int get_perfdata_limit (range * threshold, unsigned long base,
			  unsigned long long *limit, bool percent);
  int get_perfdata_limit_converted (range * threshold, unsigned long base,
				    int shift, unsigned long long *limit,
				    bool percent);

#ifdef __cplusplus
}
#endif

#endif				/* _PERFDATA_H_ */
