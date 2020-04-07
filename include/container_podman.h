/* container_podman.h -- a library for checking Podman containes via varlink

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

#ifndef _CONTAINER_PODMAN_H
#define _CONTAINER_PODMAN_H

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONTAINER_PODMAN_PRIVATE

  typedef struct podman_varlink
  {
    VarlinkConnection *connection;
    VarlinkObject *parameters;
  } podman_varlink_t;

  long podman_varlink_callback (VarlinkConnection * connection,
				const char *error, VarlinkObject * parameters,
				uint64_t flags, void *userdata);

  long podman_varlink_check_event (VarlinkConnection * connection,
				   char **err);

  int podman_varlink_get (struct podman_varlink *pv,
			  const char *varlinkmethod, char *param, char **json,
			  char **err);

#else

  struct podman_varlink;

#endif

  /* Allocates space for a new varlink object.
   * Returns 0 if all went ok. Errors are returned as negative values.  */
  int podman_varlink_new (struct podman_varlink **pv, char *varlinkaddr);

  /* Drop a reference of the varlink library context. If the refcount of
   * reaches zero, the resources of the context will be released.  */
  struct podman_varlink *podman_varlink_unref (struct podman_varlink *pv);

  int podman_running_containers (struct podman_varlink *pv,
				 unsigned int *count, const char *image,
				 char **perfdata, bool verbose);
  int podman_memory (struct podman_varlink *pv, char **perfdata,
		     bool verbose);

#ifdef __cplusplus
}
#endif

#endif				/* _CONTAINER_PODMAN_H */
