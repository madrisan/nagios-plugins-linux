/* Library for getting informations about the running processes

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _PROCESSES_H_
#define _PROCESSES_H_

#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct procs_list_node;

  /* Return name corresponding to 'uid', or NULL on error */
  char *uid_to_username (uid_t uid);

  /* Return the list of the users with running processes */
  struct procs_list_node *procs_list_getall (bool verbose);

  /* Access to procs generated lists */
  struct procs_list_node *procs_list_node_get_next (struct procs_list_node
						    *list);

  void procs_list_print_perfdata (struct procs_list_node *list);

#ifdef __cplusplus
}
#endif

#endif				/* _PROCESSES_H_ */
