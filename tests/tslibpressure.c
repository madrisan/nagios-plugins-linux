// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/pressure.c
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
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include "testutils.h"

# define NPL_TESTING
#  include "../lib/pressure.c"
# undef NPL_TESTING

static struct proc_psi_oneline *psi_oneline = NULL;
static struct proc_psi_twolines *psi_twolines = NULL;
static unsigned long long starvation[2];

static int
test_init ()
{
  const char *env_variable_cpu = "NPL_TEST_PATH_PROCPRESSURE_CPU",
	     *env_variable_io = "NPL_TEST_PATH_PROCPRESSURE_IO";

  if (setenv (env_variable_cpu, NPL_TEST_PATH_PROCPRESSURE_CPU, 1) < 0)
    return EXIT_AM_HARDFAIL;
  if (setenv (env_variable_io, NPL_TEST_PATH_PROCPRESSURE_IO, 1) < 0)
    return EXIT_AM_HARDFAIL;

  proc_psi_read_cpu (&psi_oneline, &starvation[0], 1);
  proc_psi_read_io (&psi_twolines, &starvation[0], 1);

  unsetenv (env_variable_cpu);
  unsetenv (env_variable_io);

  return 0;
}

static void
test_memory_release ()
{
  free (psi_oneline);
  free (psi_twolines);
}

typedef struct test_data
{
  unsigned long long value;
  unsigned long long expect_value;
} test_data;

static int
test_procpressure (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC (data->value, data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int err, ret = 0;

  if ((err = test_init ()) != 0)
    return err;

# define DO_TEST(DATATYPE, VALUE, EXPECT_VALUE)          \
  do                                                     \
    {                                                    \
      test_data data = {                                 \
        .value = VALUE,                                  \
        .expect_value = EXPECT_VALUE,                    \
      };                                                 \
      if (test_run("check " PATH_PSI_PROC_CPU " parser", \
                   test_procpressure, (&data)) < 0)      \
        ret = -1;                                        \
    }                                                    \
  while (0)

  /* test the proc parser */

  /* we multiply by 100 the averages to somewhat transform
   * the double values into integer ones  */
  DO_TEST ("cpu some avg10", psi_oneline->avg10 * 100, 748ULL);
  DO_TEST ("cpu some avg60", psi_oneline->avg60 * 100, 625ULL);
  DO_TEST ("cpu some avg300", psi_oneline->avg300 * 100, 666ULL);
  DO_TEST ("cpu single total", psi_oneline->total, 200932088ULL);

  DO_TEST ("io some avg10", psi_twolines->some_avg10 * 100, 8ULL);
  DO_TEST ("io some avg60", psi_twolines->some_avg60 * 100, 2ULL);
  DO_TEST ("io some avg300", psi_twolines->some_avg300 * 100, 1ULL);
  DO_TEST ("io some total", psi_twolines->some_total, 40636071ULL);

  DO_TEST ("io full avg10", psi_twolines->full_avg10 * 100, 4ULL);
  DO_TEST ("io full avg60", psi_twolines->full_avg60 * 100, 1ULL);
  DO_TEST ("io full avg300", psi_twolines->full_avg300 * 100, 0ULL);
  DO_TEST ("io full total", psi_twolines->full_total, 32897091ULL);

  test_memory_release ();

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
