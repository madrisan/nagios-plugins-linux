/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/container.c
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

#include "container.h"
#include "testutils.h"

#define NPL_TESTING
#include "../lib/container.c"
#undef NPL_TESTING

struct tstitem {
  char * image;
  char * perfdata;
  int occurrences;
} test[] = {
  [0].image = "nginx",
  [0].perfdata = "containers_nginx=3",
  [0].occurrences = 3,
  [1].image = "redis",
  [1].perfdata = "containers_redis=1",
  [1].occurrences = 1,
  [2].image = NULL,
  [2].perfdata = "containers_redis=1 containers_nginx=3 containers_total=4",
  [2].occurrences = 4
};

static int
mymain (void)
{
  int ret = 0;

  char *perfdata;
  int containers;

  for (int i = 1; i < 3; i++)
    {
      containers = docker_running_containers (test[i].image, &perfdata, false);
      TEST_ASSERT_EQUAL_NUMERIC (containers, test[i].occurrences);
      TEST_ASSERT_EQUAL_STRING (perfdata, test[i].perfdata);
      free (perfdata);
    }

  return ret;
}

TEST_MAIN (mymain);
