/* thresholds.h -- nagios thresholds

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#pragma once

#include "system.h"

/* Return codes for _set_thresholds */
#define NP_RANGE_UNPARSEABLE 1
#define NP_WARN_WITHIN_CRIT  2

#define NP_RANGE_OUTSIDE 0
#define NP_RANGE_INSIDE  1

/* see: nagios-plugins-1.4.15/lib/utils_base.h */
typedef struct range_struct
{
  double start;
  double end;
  bool start_infinity;		/* false (default) or true */
  bool end_infinity;
  int alert_on;			/* OUTSIDE (default) or INSIDE */
} range;

typedef struct thresholds_struct
{
  range *warning;
  range *critical;
} thresholds;

int get_status (double, thresholds *);
int set_thresholds (thresholds **, char *, char *);
bool thresholds_expressed_as_percentages (char *warn_string,
					  char *critical_string);
