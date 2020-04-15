/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/container_podman_stats.c
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
#include "logging.h"
#include "testutils.h"
#include "units.h"

static int podman_varlink_get (struct podman_varlink *pv,
			       const char *varlinkmethod, const char *param,
			       char **json, char **err);

#include "../lib/container_podman_stats.c"

static int
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    const char *param, char **json, char **err)
{
  char *filename = NPL_TEST_PATH_PODMAN_LISTCONTAINERS_JSON;

  dbg ("%s: the data file is %s\n", __func__, filename);
  *json = test_fstringify (filename);
  if (NULL == *json)
    return EXIT_AM_HARDFAIL;

  return 0;
}

#undef NPL_TESTING

typedef struct test_data
{
  char *image;
  char *shortid_0;
  char *shortid_1;
  unsigned int containers;
} test_data;

static int
test_podman_json_parser_list (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;
  char *shortid_0, *shortid_1;
  struct podman_varlink *pv = NULL;
  unsigned int containers;
  hashtable_t * hashtable;

  json_parser_list (pv, NULL, &hashtable);
  containers = counter_get_unique_elements (hashtable);

  shortid_0 = hashtable->keys[0];
  shortid_1 = hashtable->keys[1];

  TEST_ASSERT_EQUAL_NUMERIC (containers, data->containers);
  TEST_ASSERT_EQUAL_STRING (shortid_0, data->shortid_0);
  TEST_ASSERT_EQUAL_STRING (shortid_1, data->shortid_1);

  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(MSG, IMAGE, CONTAINERS, ID0, ID1)                  \
  do                                                               \
    {                                                              \
      test_data data = {                                           \
        .image = IMAGE,                                            \
        .containers = CONTAINERS,                                  \
	.shortid_0 = ID0,                                          \
	.shortid_1 = ID1                                           \
      };                                                           \
      if (test_run(MSG, test_podman_json_parser_list, &data) < 0)  \
        ret = -1;                                                  \
    }                                                              \
  while (0)

  DO_TEST ("check json_parser_list without image set",
	   NULL, 2, "3b395e067a30", "fced2dbe15a8");

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
