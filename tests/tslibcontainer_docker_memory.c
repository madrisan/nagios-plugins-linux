// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/container_docker_memory.c
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

#include <stdlib.h>

#include "container_docker.h"
#include "testutils.h"

struct docker_memory_desc *dockermem = NULL;

static char *
get_docker_memory_stat_path ()
{
  return NPL_TEST_PATH_SYSDOCKERMEMSTAT;
}

#define NPL_TESTING
#include "../lib/container_docker_memory.c"
#undef NPL_TESTING

static int
test_memory_init ()
{
  int ret = 0;

  ret = docker_memory_desc_new (&dockermem);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  docker_memory_desc_read (dockermem);

  return 0;
}

static void
test_memory_release ()
{
  docker_memory_desc_unref (dockermem);
}

typedef struct test_data
{
  long long value;
  long long expect_value;
} test_data;

static int
test_sysmeminfo (const void *tdata)
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

  if ((err = test_memory_init ()) != 0)
    return err;

  struct docker_memory_desc *sysdata = dockermem;

# define DO_TEST(MEMTYPE, VALUE, EXPECT_VALUE)                           \
  do                                                                     \
    {                                                                    \
      test_data data = {                                                 \
	.value = VALUE,                                                  \
	.expect_value = EXPECT_VALUE,                                    \
      };                                                                 \
      if (test_run("check docker memory parser for " MEMTYPE " memory",  \
		   test_sysmeminfo, (&data)) < 0)                        \
	ret = -1;                                                        \
    }                                                                    \
  while (0)

  /* test the sysfs data parser */

  DO_TEST ("total_cache", sysdata->b_total_cache, 108507136L);
  DO_TEST ("total_rss", sysdata->b_total_rss, 53219328L);
  DO_TEST ("total_swap", sysdata->b_total_swap, 0L);
  DO_TEST ("total_unevictable", sysdata->b_total_unevictable, 0L);
  DO_TEST ("total_pgfault", sysdata->c_total_pgfault, 97485L);
  DO_TEST ("total_pgmajfault", sysdata->c_total_pgmajfault, 424L);
  DO_TEST ("total_pgpgin", sysdata->c_total_pgpgin, 111725L);
  DO_TEST ("total_pgpgout", sysdata->c_total_pgpgout, 72238L);

  test_memory_release ();

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
