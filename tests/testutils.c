/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Basic test utilities for the Nagios Plugin Linux.
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
 *
 * This software is based on the source code of the tool "vmstat".
 */

#include <stdio.h>

#include "common.h"
#include "testutils.h"

static size_t test_counter;

int
test_main (int argc, char **argv, int (*func) (void), ...)
{
  int ret;
  ret = (func) ();

  return ret;
}

int
test_run (const char *title, int (*body) (const void *data), const void *data)
{
  int ret = body (data);
  test_counter++;

  fprintf (stderr, "%2zu) %-65s ... ", test_counter, title);

  if (ret == 0)
    fprintf (stderr, "OK\n");
  else if (ret == EXIT_AM_SKIP)
    fprintf (stderr, "SKIP\n");
  else
    fprintf (stderr, "FAILED\n");

  return ret;
}
