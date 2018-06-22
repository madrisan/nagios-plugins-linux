/* container.h -- a library for checking sysfs for Docker exposed metrics

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

#ifndef _CONTAINER_H
#define _CONTAINER_H

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

  unsigned int docker_running_containers (const char *image,
					  char **perfdata, bool verbose);

#ifdef __cplusplus
}
#endif

#endif				/* _CONTAINER_H */
