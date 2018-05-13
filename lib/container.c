/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking sysfs for Docker exposed metrics.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "sysfsparser.h"

#define PATH_SYS_CGROUP		"/sys/fs/cgroup"
#define PATH_SYS_PIDS_DOCKER	PATH_SYS_CGROUP	"/pids/docker"

static bool
docker_metrics_exist ()
{
  return sysfsparser_path_exist (PATH_SYS_PIDS_DOCKER);
}

/* Returns the number of running Docker containers  */
int
docker_running_containers_number (bool verbose)
{
  DIR *dirp;
  struct dirent *dp;
  int containers = 0;

  if (false == docker_metrics_exist ())
    return 0;

  sysfsparser_opendir(&dirp, PATH_SYS_PIDS_DOCKER);

  /* Scan entries under PATH_SYS_PIDS_DOCKER */
  while ((dp = sysfsparser_readfilename(dirp, DT_DIR)))
    {
      containers ++;

      dbg ("found docker container \"%s\"\n", dp->d_name);

      //if (!verbose)
      //  continue;
    }

  sysfsparser_closedir (dirp);

  return containers;
}
