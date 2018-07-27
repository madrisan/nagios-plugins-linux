/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/container_count.c
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

#include "container.h"
#include "testutils.h"

#define NPL_TESTING

static int docker_get (chunk_t * chunk, const int query);
static void docker_close (chunk_t * chunk);

#include "../lib/container_count.c"

static int
docker_get (chunk_t * chunk, const int query)
{
  char *filename = NULL;

  switch (query)
    {
      default:
	return EXIT_AM_HARDFAIL;
	break;
      case DOCKER_CONTAINERS_JSON:
	filename = NPL_TEST_PATH_CONTAINER_JSON;
	break;
    }

  if (NULL == filename)
    return EXIT_AM_HARDFAIL;

  chunk->memory = test_fstringify (filename);
  if (NULL == chunk->memory)
    return EXIT_AM_HARDFAIL;

  chunk->size = strlen (chunk->memory);

  return 0;
}

static void
docker_close (chunk_t * chunk)
{
  free (chunk->memory);
}
#undef NPL_TESTING

typedef struct test_data
{
  char * image;
  char * perfdata;
  unsigned int expect_value;
} test_data;

static int
test_docker_running_containers (const void *tdata)
{
  const struct test_data *data = tdata;
  int err, ret = 0;
  char * perfdata;
  unsigned int containers;

  err = docker_running_containers (&containers, data->image, &perfdata, false);
  if (err != 0)
    {
      free (perfdata);
      return err;
    }

  TEST_ASSERT_EQUAL_NUMERIC (containers, data->expect_value);
  TEST_ASSERT_EQUAL_STRING (perfdata, data->perfdata);

  free (perfdata);
  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(MSG, IMAGE, PERFDATA, EXPECT_VALUE)                  \
  do                                                                 \
    {                                                                \
      test_data data = {                                             \
        .image = IMAGE,                                              \
        .perfdata = PERFDATA,                                        \
        .expect_value = EXPECT_VALUE                                 \
      };                                                             \
      if (test_run(MSG, test_docker_running_containers, &data) < 0)  \
        ret = -1;                                                    \
    }                                                                \
  while (0)

  DO_TEST ("check running docker containers with image set", "nginx",
	   "containers_nginx=3", 3);
  DO_TEST ("check running docker containers with image set", "redis",
	   "containers_redis=1", 1);
  DO_TEST ("check running docker containers", NULL,
	   "containers_redis=1 containers_nginx=3 containers_total=4", 4);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
