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

#include "config.h"

#include "common.h"
#include "nputils.h"

const char *
state_text (enum nagios_status result)
{
  switch (result)
    {
    case STATE_OK:
      return "OK";
    case STATE_WARNING:
      return "WARNING";
    case STATE_CRITICAL:
      return "CRITICAL";
    case STATE_DEPENDENT:
      return "DEPENDENT";
    default:
      return "UNKNOWN";
    }
}
