/*
 * License: GPL
 * Copyright (c) 2006 Nagios Plugins Development Team
 *
 * Library of useful functions for nagios plugins
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system.h"
#include "thresholds.h"
#include "xalloc.h"

#define OUTSIDE 0
#define INSIDE  1

/*
 * Returns TRUE if alert should be raised based on the range 
 */
int
check_range (double value, range * my_range)
{
  bool no = false;
  bool yes = true;

  if (my_range->alert_on == INSIDE)
    {
      no = true;
      yes = false;
    }

  if (my_range->end_infinity == false && my_range->start_infinity == false)
    {
      if ((my_range->start <= value) && (value <= my_range->end))
	return no;
      else
	return yes;
    }
  else if (my_range->start_infinity == false
	   && my_range->end_infinity == true)
    {
      if (my_range->start <= value)
	return no;
      else
	return yes;
    }
  else if (my_range->start_infinity == true
	   && my_range->end_infinity == false)
    {
      if (value <= my_range->end)
	return no;
      else
	return yes;
    }
  else
    return no;
}

int
get_status (double value, thresholds * my_thresholds)
{
  if (my_thresholds->critical != NULL)
    {
      if (check_range (value, my_thresholds->critical) == true)
	return STATE_CRITICAL;
    }
  if (my_thresholds->warning != NULL)
    {
      if (check_range (value, my_thresholds->warning) == true)
	return STATE_WARNING;
    }
  return STATE_OK;
}

void
set_range_start (range * this, double value)
{
  this->start = value;
  this->start_infinity = false;
}

void
set_range_end (range * this, double value)
{
  this->end = value;
  this->end_infinity = false;
}

range *
parse_range_string (char *str)
{
  range *temp_range;
  double start;
  double end;
  char *end_str;

  temp_range = (range *) xmalloc (sizeof (range));

  /*
   * Set defaults 
   */
  temp_range->start = 0;
  temp_range->start_infinity = false;
  temp_range->end = 0;
  temp_range->end_infinity = true;
  temp_range->alert_on = OUTSIDE;

  if (str[0] == '@')
    {
      temp_range->alert_on = INSIDE;
      str++;
    }

  end_str = index (str, ':');
  if (end_str != NULL)
    {
      if (str[0] == '~')
	temp_range->start_infinity = true;
      else
	{
	  start = strtod (str, NULL);	/* Will stop at the ':' */
	  set_range_start (temp_range, start);
	}
      end_str++;		/* Move past the ':' */
    }
  else
    {
      end_str = str;
    }
  end = strtod (end_str, NULL);
  if (strcmp (end_str, "") != 0)
    set_range_end (temp_range, end);

  if (temp_range->start_infinity == true ||
      temp_range->end_infinity == true ||
      temp_range->start <= temp_range->end)
    {
      return temp_range;
    }
  free (temp_range);
  return NULL;
}

/*
 * returns 0 if okay, otherwise 1 
 */
int
set_thresholds (thresholds ** my_thresholds, char *warn_string,
		char *critical_string)
{
  thresholds *temp_thresholds = NULL;

  if ((temp_thresholds = malloc (sizeof (thresholds))) == NULL)
    {
      printf ("Cannot allocate memory: %s", strerror (errno));
      exit (STATE_UNKNOWN);
    }

  temp_thresholds->warning = NULL;
  temp_thresholds->critical = NULL;

  if (warn_string != NULL)
    {
      if ((temp_thresholds->warning =
	   parse_range_string (warn_string)) == NULL)
	{
	  free (temp_thresholds);
	  return NP_RANGE_UNPARSEABLE;
	}
    }
  if (critical_string != NULL)
    {
      if ((temp_thresholds->critical =
	   parse_range_string (critical_string)) == NULL)
	{
	  free (temp_thresholds);
	  return NP_RANGE_UNPARSEABLE;
	}
    }

  *my_thresholds = temp_thresholds;

  return 0;
}
