/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/url_encode.c
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
#include "url_encode.h"

#define NPL_TESTING

#include "../lib/url_encode.c"

typedef struct test_data
{
  char * url;
  char * expect_value;
} test_data;

static int
test_url_encoding (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;
  char *encoded_url = url_encode (data->url);

  TEST_ASSERT_EQUAL_STRING (encoded_url, data->expect_value);
  free (encoded_url);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(URL, EXPECT_VALUE)                  \
  do                                                \
    {                                               \
      test_data data = {                            \
	.url = URL,                                 \
	.expect_value = EXPECT_VALUE,               \
      };                                            \
      if (test_run("check url encoding for " URL,   \
		   test_url_encoding, (&data)) < 0) \
	ret = -1;                                   \
    }                                               \
  while (0)

  DO_TEST ("?test=true", "%3ftest%3dtrue");
  DO_TEST ("?test=true&debug=false", "%3ftest%3dtrue%26debug%3dfalse");
  DO_TEST ("{\"status\":{\"running\":true}}",
	   "%7b%22status%22%3a%7b%22running%22%3atrue%7d%7d");

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
