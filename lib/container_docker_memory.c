// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking sysfs for Docker exposed metrics for memory.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysfsparser.h"

#include "common.h"
#include "container_docker.h"
#include "logging.h"
#include "messages.h"
#include "xasprintf.h"

#ifndef NPL_TESTING

#define PATH_SYS_CGROUP "/sys/fs/cgroup"
#define PATH_SYS_DOCKER_MEM PATH_SYS_CGROUP "/memory/docker"

static char *
get_docker_memory_stat_path ()
{
  char *syspath = NULL;

  /* Debian 8.10 and 9.4, Fedora 28 */
  if (sysfsparser_path_exist (PATH_SYS_DOCKER_MEM "/memory.stat"))
    syspath = xasprintf ("%s/memory.stat", PATH_SYS_DOCKER_MEM);

  return syspath;
}

#endif		/* NPL_TESTING */

struct docker_memory_desc
{
  int refcount;

  long long b_total_cache;
  long long b_total_rss;
  long long b_total_swap;
  long long b_total_unevictable;
  long long c_total_pgfault;
  long long c_total_pgmajfault;
  long long c_total_pgpgin;
  long long c_total_pgpgout;
};

/* Allocates space for a new docker_memory_desc object.
 * Returns 0 if all went ok. Errors are returned as negative values.  */
int
docker_memory_desc_new (struct docker_memory_desc **memdesc)
{
  struct docker_memory_desc *d;

  d = calloc (1, sizeof (struct docker_memory_desc));
  if (!d)
    return -ENOMEM;

  d->refcount = 1;

  *memdesc = d;
  return 0;
}

/* Fill the docker_memory_desc structure pointed with the values found in the
 * sysfs filesystem */
void
docker_memory_desc_read (struct docker_memory_desc *__restrict memdesc)
{
  char *line = NULL;
  FILE *fp;
  size_t len = 0;
  ssize_t chread;

  char *syspath = get_docker_memory_stat_path ();

  if (NULL == syspath)
    plugin_error (STATE_UNKNOWN, errno, "sysfs file not found: memory.stat");

  if ((fp = fopen (syspath, "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", syspath);

  dbg ("parsing the file \"%s\"...\n", syspath);
  while ((chread = getline (&line, &len, fp)) != -1)
    {
      dbg ("line: %s", line);

      if (sysfsparser_linelookup_numeric
	    (line, "total_cache", &memdesc->b_total_cache));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_rss", &memdesc->b_total_rss));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_swap", &memdesc->b_total_swap));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_unevictable", &memdesc->b_total_unevictable));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_pgfault", &memdesc->c_total_pgfault));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_pgmajfault", &memdesc->c_total_pgmajfault));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_pgpgin", &memdesc->c_total_pgpgin));
      else
	if (sysfsparser_linelookup_numeric
	    (line, "total_pgpgout", &memdesc->c_total_pgpgout));
      else
	continue;
    }

  fclose (fp);
}

/* Accessing the values from docker_memory_desc */

#define docker_memory_desc_get(arg, prefix) \
  long long docker_memory_get_ ## arg (struct docker_memory_desc *p)        \
    {                                                                       \
      dbg(" -> %s: %lld\n", #arg, (p == NULL) ? 0 : p->prefix ## _ ## arg); \
      return (p == NULL) ? 0 : p->prefix ## _ ## arg;                       \
    }
#define docker_memory_desc_get_bytes(arg) \
   docker_memory_desc_get(arg, b)
#define docker_memory_desc_get_count(arg) \
   docker_memory_desc_get(arg, c)

  docker_memory_desc_get_bytes (total_cache);
  docker_memory_desc_get_bytes (total_rss);
  docker_memory_desc_get_bytes (total_swap);
  docker_memory_desc_get_bytes (total_unevictable);
  docker_memory_desc_get_count (total_pgfault);
  docker_memory_desc_get_count (total_pgmajfault);
  docker_memory_desc_get_count (total_pgpgin);
  docker_memory_desc_get_count (total_pgpgout);

#undef docker_memory_desc_get_count
#undef docker_memory_desc_get_bytes
#undef docker_memory_desc_get

/* Drop a reference of the docker_memory_desc library context. If the refcount
 * of reaches zero, the resources of the context will be released.  */
struct docker_memory_desc *
docker_memory_desc_unref (struct docker_memory_desc *memdesc)
{
  if (memdesc == NULL)
    return NULL;

  memdesc->refcount--;
  if (memdesc->refcount > 0)
    return memdesc;

  free (memdesc);
  return NULL;
}
