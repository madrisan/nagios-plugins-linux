/*
 * License: GPLv3+
 * Copyright (c) 2016,2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_cswch.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "cpustats.h"
#include "system.h"
#include "testutils.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__((unused));
static _Noreturn void usage (FILE * out) __attribute__((unused));

#define NPL_TESTING
# include "../plugins/check_cswch.c"
#undef NPL_TESTING

static bool
test_cswch_proc_stat_exists ()
{
  FILE *fp;

  if ((fp = fopen (get_path_proc_stat (), "r")) == NULL)
    return false;

  fclose (fp);
  return true;
}

static int
test_cswch_monotonic (const void *tdata)
{
  long delta = get_ctxtdelta (1, 1, false);

  if (delta <= 0)
    return -1;

  return 0;
}

static int
test_cswch_proc_parsing ()
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCSTAT";

  ret = setenv (env_variable, NPL_TEST_PATH_PROCSTAT, 1);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  /* next function will parse the line "ctxt 13817032" */
  if (cpu_stats_get_cswch () != 13817032)
    ret = -1;

  unsetenv (env_variable);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

  if (!test_cswch_proc_stat_exists ())
    return EXIT_AM_SKIP;

  if (test_run ("check if get_ctxtdelta() is monotonic",
		test_cswch_monotonic, NULL) < 0)
    ret = -1;
  if (test_run ("check for parsing errors in cpu_stats_get_cswch()",
		test_cswch_proc_parsing, NULL) != 0)
    ret = -1;

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
