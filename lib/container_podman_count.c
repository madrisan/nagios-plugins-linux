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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE		/* activate extra prototypes for glibc */
#endif

#include <assert.h>
#ifndef NPL_TESTING
# include <varlink.h>
#endif
#include <string.h>

#include "common.h"
#include "collection.h"
#include "container_podman.h"
#include "json_helpers.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "xalloc.h"
#include "xasprintf.h"

/* Hide all jsmn API symbols by making them static */
#define JSMN_STATIC
#include "jsmn.h"

/* parse the json stream and return a pointer to the hashtable containing
   the values of the discovered 'tokens', or return NULL if the data
   cannot be parsed;

   the format of the data follows:

       {
         "containerS": [
           {
             "command": [
               "docker-entrypoint.sh",
               "redis-server"
             ],
             "containerrunning": false,
             "createdat": "2020-03-28T15:26:32+01:00",
             "id": "5250ca78f8a28eeed8ebcd1c92c4fac9be8afc343c15071ed7a5b0a31c7d48db",
             "image": "docker.io/library/redis:latest",
             "imageid": "f0453552d7f26fc38ffc05fa034aa7a7bc6fbb01bc7bc5a9e4b3c0ab87068627",
             "mounts": [
                ...
             ],
             "names": "srv-redis-1",
             ...
             "status": "exited"
           },
           {
             ...
           },
           {
             ...
           },
         ]
       }	*/

static void
json_parser (char *json, const char *root_key, hashtable_t ** ht_running,
	     hashtable_t ** ht_exited)
{
  jsmntok_t *tokens;
  size_t i, ntoken, level = 0;

  char *keys[] = { "containerrunning", "id", "image", "names", "status" },
    *vals[] = { NULL, NULL, NULL, NULL, NULL },
    **containerrunning = &vals[0], **image = &vals[2];
  size_t keys_num = sizeof (keys) / sizeof (char *);

  tokens = json_tokenise (json, &ntoken);
  if (NULL == tokens)
    plugin_error (STATE_UNKNOWN, 0, "invalid or corrupted JSON data");

  dbg ("number of json tokens: %lu\n", ntoken);

  *ht_exited = counter_create ();
  *ht_running = counter_create ();

  for (i = 0; i < ntoken; i++)
    {
      jsmntok_t *t = &tokens[i];

      // should never reach uninitialized tokens
      assert (t->start != -1 && t->end != -1);

      dbg ("[%lu] %s: \"%.*s\"\n", i,
	   (t->type == JSMN_ARRAY) ? "JSMN_ARRAY" :
	   ((t->type == JSMN_OBJECT) ? "JSMN_OBJECT" :
	    ((t->type == JSMN_STRING) ? "JSMN_STRING" :
	     ((t->type ==
	       JSMN_UNDEFINED) ? "JSMN_UNDEFINED" : "JSMN_PRIMITIVE"))),
	   t->end - t->start, json + t->start);

      switch (t->type)
	{
	case JSMN_OBJECT:
	  level++;		/* FIXME: should be decremented at each object end */
	  break;

	case JSMN_STRING:
	  if ((1 == level) && (0 != json_token_streq (json, t, root_key)))
	    plugin_error (STATE_UNKNOWN, 0,
			  "%s: expected string \"%s\" not found",
			  root_key, __func__);
	  for (size_t j = 0; j < keys_num; j++)
	    {
	      if (0 == json_token_streq (json, t, keys[j]))
		{
		  vals[j] = json_token_tostr (json, &tokens[++i]);
		  dbg
		    ("found token \"%s\" with value \"%s\" at position %d\n",
		     keys[j], vals[j], t->start);
		  break;
		}
	    }
	  break;

	default:
	  if (0 == level)
	    plugin_error (STATE_UNKNOWN, 0,
			  "%s: root element must be an object", __func__);
	}

      if (podman_array_is_full (vals, keys_num))
	{
	  if (STREQ (*containerrunning, "true"))
	    counter_put (*ht_running, *image, 1);
	  else
	    counter_put (*ht_exited, *image, 1);
	  dbg ("new container found:\n");
	  for (size_t j = 0; j < keys_num; j++)
	    {
	      dbg (" * \"%s\": \"%s\"\n", keys[j], vals[j]);
	      free (vals[j]);
	      vals[j] = NULL;
	    }
	}
    }

  free (tokens);
}

int
podman_running_containers (struct podman_varlink *pv, unsigned int *count,
			   const char *image, char **perfdata)
{
  const char *varlinkmethod = "io.podman.GetContainersByStatus";

  hashtable_t *ht_running, *ht_exited;
  char *errmsg = NULL, *json;
  unsigned int running_containers = 0, exited_containers = 0;
  int ret;

  ret = podman_varlink_get (pv, varlinkmethod, NULL, &json, &errmsg);
  if (ret < 0)
    {
      podman_varlink_unref (pv);
      plugin_error (STATE_UNKNOWN, 0, "%s", errmsg);
    }
  dbg ("varlink %s returned: %s", varlinkmethod, json);

  json_parser (json, "containerS", &ht_running, &ht_exited);

  if (image)
    {
      char *image_norm = podman_image_name_normalize (image);
      hashable_t *np_exited = counter_lookup (ht_exited, image),
	*np_running = counter_lookup (ht_running, image);

      exited_containers = np_exited ? np_exited->count : 0;

      running_containers = np_running ? np_running->count : 0;
      *perfdata =
	xasprintf ("containers_exited_%s=%u containers_running_%s=%u",
		   image_norm, exited_containers,
		   image_norm, running_containers);
      free (image_norm);
    }
  else
    {
      exited_containers = counter_get_elements (ht_exited);
      running_containers = counter_get_elements (ht_running);

      size_t size;
      FILE *stream = open_memstream (perfdata, &size);
      for (unsigned int j = 0; j < ht_running->uniq; j++)
	{
 	  char *image_norm = podman_image_name_normalize (ht_running->keys[j]);
	  hashable_t *np = counter_lookup (ht_running, ht_running->keys[j]);
	  assert (NULL != np);
	  fprintf (stream, "%s=%lu ", image_norm, np->count);
	  free (image_norm);
	}
      fprintf (stream,
	       "containers_exited=%u containers_running=%u",
	       exited_containers, ht_running->elements);
      fclose (stream);
    }

  dbg ("running containers: %u, exited: %u \n",
       running_containers, exited_containers);
  *count = running_containers;

  counter_free (ht_running);
  counter_free (ht_exited);
  free (errmsg);

  return 0;
}
