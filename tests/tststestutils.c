/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for testutils.c
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
#include "testutils.h"

typedef struct test_data
{
  char * filename;
  size_t expect_size;
} test_data;

static int
test_fstringify_bufsize (const void *tdata)
{
  int ret = 0;
  const struct test_data *data = tdata;

  char * buffer = test_fstringify (data->filename);

  TEST_ASSERT_EQUAL_NUMERIC (data->expect_size, strlen (buffer));

  free (buffer);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;
  char *filename = NPL_TEST_PATH_CONTAINER_JSON;

#define DO_TEST(MSG, FILENAME, EXPECT_SIZE)                   \
  do                                                          \
    {                                                         \
      test_data data = {                                      \
	.filename = FILENAME,                                 \
	.expect_size = EXPECT_SIZE,                           \
      };                                                      \
      if (test_run(MSG, test_fstringify_bufsize, &data) < 0)  \
	ret = -1;                                             \
    }                                                         \
  while (0)

  DO_TEST ("checking test_fstringify ()", filename, 3832);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
