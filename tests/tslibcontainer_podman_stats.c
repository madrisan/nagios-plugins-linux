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

static int podman_varlink_get (struct podman_varlink *pv,
			       const char *varlinkmethod, const char *param,
			       char **json, char **err);

#include "../lib/container_podman_stats.c"

static int
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    const char *param, char **json, char **err)
{
  char *filename = NPL_TEST_PATH_PODMAN_GETCONTAINERSTATS_JSON;

  dbg ("%s: the data file is %s\n", __func__, filename);
  *json = test_fstringify (filename);
  if (NULL == *json)
    return EXIT_AM_HARDFAIL;

  return 0;
}

#undef NPL_TESTING

typedef struct test_data
{
  char *id;
  unsigned long container_memory;
} test_data;

static int
test_podman_json_parser_stats (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;
  /* the id is ignored by json_parser_stats in test mode */
  char *id = NULL; /* *id = ((test_data *)tdata)->id; */
  struct podman_varlink *pv = NULL;
  unsigned long container_memory;

  json_parser_stats (pv, id, &container_memory);

  TEST_ASSERT_EQUAL_NUMERIC (container_memory, data->container_memory);

  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(MSG, ID, MEMORY)                                   \
  do                                                               \
    {                                                              \
      test_data data = {                                           \
        .id = ID,                                                  \
        .container_memory = MEMORY                                 \
      };                                                           \
      if (test_run(MSG, test_podman_json_parser_stats, &data) < 0) \
        ret = -1;                                                  \
    }                                                              \
  while (0)

  DO_TEST ("check container memory executing json_parser_stats",
	   "fced2dbe15a84", 9224192);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain);
