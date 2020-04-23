/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/container_podman_count.c
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
#define _GNU_SOURCE		/* activate extra prototypes for glibc */
#endif

#include <stdlib.h>

#define NPL_TESTING

#include "container_podman.h"
#include "testutils.h"

static int podman_varlink_get (struct podman_varlink *pv,
			       const char *varlinkmethod, char **json,
			       char **err);

#include "../lib/container_podman.c"
#include "../lib/container_podman_count.c"

static int 
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    char **json, char **err)
{ 
  char *filename = NPL_TEST_PATH_PODMAN_GETCONTAINERSBYSTATUS_JSON;

  *json = test_fstringify (filename);
  if (NULL == *json)
    return EXIT_AM_HARDFAIL;

  return 0;
}

#undef NPL_TESTING

typedef struct test_data
{
  char * image;
  char * perfdata;
  unsigned int expect_value;
} test_data;

static int
test_podman_running_containers (const void *tdata)
{
  const struct test_data *data = tdata;
  int err, ret = 0;
  char * perfdata = NULL;
  struct podman_varlink *pv = NULL;
  unsigned int containers;

  err = podman_running_containers (pv, &containers, data->image, &perfdata);
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
      if (test_run(MSG, test_podman_running_containers, &data) < 0)  \
        ret = -1;                                                    \
    }                                                                \
  while (0)

  DO_TEST ("check running podman containers with image set",
	   "docker.io/library/nginx:latest",
	   "containers_exited_nginx_latest=3 containers_running_nginx_latest=2", 2);
  DO_TEST ("check running podman containers with image set",
	   "docker.io/library/redis:latest",
	   "containers_exited_redis_latest=1 containers_running_redis_latest=1", 1);
  DO_TEST ("check running podman containers", NULL,
	   "nginx_latest=2 redis_latest=1 containers_exited=4 containers_running=3", 3);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
