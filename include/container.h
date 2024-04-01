// SPDX-License-Identifier: GPL-3.0-or-later
/* container_docker.h -- a library for checking sysfs for Docker exposed metrics

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _CONTAINER_DOCKER_H
#define _CONTAINER_DOCKER_H

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct chunk
  {
    char *memory;
    size_t size;
  } chunk_t;

  struct docker_memory_desc;

  /* Allocates space for a new docker_memory_desc object.
   * Returns 0 if all went ok. Errors are returned as negative values.  */
  int docker_memory_desc_new (struct docker_memory_desc **memdesc);

  /* Fill the docker_memory_desc structure pointed with the values found in the
   * sysfs filesystem.  */
  void docker_memory_desc_read (struct docker_memory_desc *__restrict
				memdesc);

  /* Drop a reference of the docker_memory_desc library context. If the refcount
   * of reaches zero, the resources of the context will be released.  */
  struct docker_memory_desc
    *docker_memory_desc_unref (struct docker_memory_desc *memdesc);

  /* Accessing the values from docker_memory_desc */

  /* Return the number of bytes of page cache memory.  */
  long long docker_memory_get_total_cache (
    struct docker_memory_desc *memdesc);

  /* Return the number of bytes of anonymous and swap cache memory
   * (includes transparent hugepages).  */
  long long docker_memory_get_total_rss (
    struct docker_memory_desc *memdesc);

  /* Return the number of bytes of swap usage.  */
  long long docker_memory_get_total_swap (
    struct docker_memory_desc *memdesc);

  /* Return the number of bytes of bytes of memory that cannot be reclaimed
   * (mlocked etc).  */
  long long docker_memory_get_total_unevictable (
    struct docker_memory_desc *memdesc);

  /* Additional metrics that may be valuable in investigating performance or
   * stability issues include page faults, which can represent either
   * segmentation faults or fetching data from disk instead of memory
   * (pgfault and pgmajfault, respectively).  */
  long long docker_memory_get_total_pgfault (
    struct docker_memory_desc *memdesc);
  long long docker_memory_get_total_pgmajfault (
    struct docker_memory_desc *memdesc);

  long long docker_memory_get_total_pgpgin (
    struct docker_memory_desc *memdesc);
  long long docker_memory_get_total_pgpgout (
    struct docker_memory_desc *memdesc);

  int docker_running_containers (char *socket, unsigned int *count,
		  		 const char *image, char **perfdata,
				 bool verbose);
  int docker_running_containers_memory (char *socket,
					long long unsigned int *memory,
					bool verbose);

#ifdef __cplusplus
}
#endif

#endif				/* _CONTAINER_DOCKER_H */
