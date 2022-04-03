// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking for Podman metrics via varlink calls.
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

#include <assert.h>
#include <varlink.h>
#include <string.h>

#include "collection.h"
#include "common.h"
#include "container_podman.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"

int
podman_running_containers (struct podman_varlink *pv, unsigned int *count,
			   const char *image, char **perfdata)
{
  char *errmsg = NULL;
  long ret;
  size_t size;
  unsigned int configured_containers, exited_containers,
	       running_containers;
  unsigned long elements, i;

  FILE *stream = open_memstream (perfdata, &size);
  hashtable_t *ht_running;
  VarlinkArray *list;

  ht_running = counter_create ();

  ret = podman_varlink_list (pv, &list, &errmsg);
  if (ret < 0)
    plugin_error (STATE_UNKNOWN, 0, "varlink_varlink_list: %s", errmsg);

  elements = varlink_array_get_n_elements (list);
  dbg ("varlink has detected %lu containers\n", elements);

  configured_containers = exited_containers = running_containers = 0;
  for (i = 0; i < elements; i++)
    {
      bool cnt_running;
      const char *cnt_id, *cnt_image, *cnt_status;
      VarlinkObject *state;

      varlink_array_get_object (list, i, &state);
      varlink_object_get_string (state, "id", &cnt_id);
      varlink_object_get_string (state, "image", &cnt_image);
      varlink_object_get_bool (state, "containerrunning", &cnt_running);
      varlink_object_get_string (state, "status", &cnt_status);

      dbg ("podman container %s\n", cnt_id);
      dbg (" * container image: %s\n", cnt_image);
      dbg (" * container is running: %s (status: %s)\n",
	   cnt_running ? "yes" : "no", cnt_status);

      /* discard containers non running the selected image,
       * when ahs been specified at command-line */
      if (image && STRNEQ (cnt_image, image))
	continue;

      if (cnt_running)
	{
	  counter_put (ht_running, cnt_image, 1);
	  running_containers++;
	}
      else if (STREQ (cnt_status, "configured"))
	configured_containers++;
      else if (STREQ (cnt_status, "exited"))
	exited_containers++;
    }

  dbg ("running containers: %u, configured: %u, exited: %u\n",
       running_containers, configured_containers, exited_containers);

  if (image)
    fprintf (stream, "configured=%u exited=%u running=%u",
	     configured_containers, exited_containers,
	     running_containers);
  else
    for (unsigned int j = 0; j < ht_running->uniq; j++)
      {
	char *image_norm =
	  podman_image_name_normalize (ht_running->keys[j]);
	hashable_t *np = counter_lookup (ht_running, ht_running->keys[j]);
	assert (NULL != np);
	fprintf (stream, "%s=%lu ", image_norm, np->count);
	free (image_norm);
      }

  *count = running_containers;

  fclose (stream);
  counter_free (ht_running);
  free (errmsg);

  return 0;
}
