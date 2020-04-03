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

#define EPOLL_TIMEOUT 100

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <varlink.h>
#include <string.h>
#include <sys/epoll.h>

#include "common.h"
#include "collection.h"
#include "json_helpers.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "xalloc.h"
#include "xasprintf.h"

/* Hide all jsmn API symbols by making them static */
#define JSMN_STATIC
#include "jsmn.h"

typedef struct podman_varlink
{
  VarlinkConnection *connection;
  VarlinkObject *parameters;
} podman_varlink_t;

static long
podman_varlink_check_event (VarlinkConnection * connection, char **err)
{
  struct epoll_event events;
  int epollfd;
  long nfds;
  long ret = 0;

  epollfd = epoll_create1 (EPOLL_CLOEXEC);
  if (epollfd == -1)
    {
      *err = xasprintf ("epoll_create1: %s (errno: %d)",
			strerror (errno), errno);
      return errno;
    }

  events.events = varlink_connection_get_events (connection);
  events.data.ptr = connection;

  ret = epoll_ctl (epollfd, EPOLL_CTL_ADD,
		   varlink_connection_get_fd (connection), &events);
  if (ret == -1)
    {
      *err =
	xasprintf ("epoll_ctrl: %s (errno: %d)", strerror (errno), errno);
      ret = errno;
      goto free_fd;
    }

  for (;;)
    {
      nfds = epoll_wait (epollfd, &events, 1, EPOLL_TIMEOUT);
      if (nfds == -1)
	{
	  *err = xasprintf ("epoll_wait: %s (errno: %d)",
			    strerror (errno), errno);
	  ret = errno;
	  break;
	}
      else if (!nfds)
	continue;
      else
	{
	  ret = varlink_connection_process_events (connection, events.events);
	  if (ret < 0)
	    *err = xasprintf ("varlink_connection_process_events: %s",
			      varlink_error_string (labs (ret)));
	  break;
	}
    }

free_fd:
  close (epollfd);

  return ret;
}

static long
podman_varlink_callback (VarlinkConnection * connection,
			 const char *error,
			 VarlinkObject * parameters,
			 uint64_t flags, void *userdata)
{
  VarlinkObject **out = userdata;
  long ret = 0;

  if (error)
    ret = -1;
  else
    *out = varlink_object_ref (parameters);

  return ret;
}

/* Allocates space for a new varlink object.
 * Returns 0 if all went ok. Errors are returned as negative values. */

int
podman_varlink_new (struct podman_varlink **pv, char *varlinkaddr)
{
  long ret;
  struct podman_varlink *v;
  VarlinkConnection *connection;

  if (NULL == varlinkaddr)
    varlinkaddr = VARLINK_ADDRESS;

  ret = varlink_connection_new (&connection, varlinkaddr);
  if (ret < 0)
    plugin_error (STATE_UNKNOWN, errno,
		  "varlink_connection_new: %s",
		  varlink_error_string (labs (ret)));

  v = calloc (1, sizeof (struct podman_varlink));
  if (!v)
    {
      varlink_connection_free (connection);
      return -ENOMEM;
    }

  v->connection = connection;
  v->parameters = NULL;
  *pv = v;

  return 0;
}

struct podman_varlink *
podman_varlink_unref (struct podman_varlink *pv)
{
  if (pv == NULL)
    return NULL;

  varlink_connection_free (pv->connection);
  if (pv->parameters)
    varlink_object_unref (pv->parameters);

  free (pv);
  return NULL;
}

static char *
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    char **err)
{
  char *result = NULL;
  long ret;
  VarlinkObject *out;

  assert (pv->connection);

  ret = varlink_object_new (&pv->parameters);
  if (ret < 0)
    {
      *err =
	xasprintf ("varlink_object_new: %s",
		   varlink_error_string (labs (ret)));
      return NULL;
    }
  assert (pv->parameters);

  ret =
    varlink_connection_call (pv->connection, varlinkmethod, pv->parameters, 0,
			     podman_varlink_callback, &out);
  if (ret < 0)
    {
      *err =
	xasprintf ("varlink_connection_call: %s",
		   varlink_error_string (labs (ret)));
      return NULL;
    }

  ret = podman_varlink_check_event (pv->connection, err);
  if (ret < 0)
    return NULL;

  ret = varlink_object_to_json (out, &result);
  if (ret < 0)
    {
      varlink_object_unref (out);
      return NULL;
    }

  return result;
}

static bool
array_is_full (char *vals[], size_t keys_num)
{
  for (size_t i = 0; i < keys_num && vals; i++)
    {
      dbg ("var[%lu] = %s\n", i, vals[i]);
      if (NULL == vals[i])
	return false;
    }
  return true;
}

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
       }   */

static void
json_parser (char *json, hashtable_t ** ht_running, hashtable_t ** ht_exited)
{
  jsmntok_t *tokens;
  size_t i, ntoken, level = 0;

  char *keys[] = { "containerrunning", "image", "names", "status" },
    *vals[] = { NULL, NULL, NULL, NULL },
    **containerrunning = &vals[0], **image = &vals[1];
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
	  if ((1 == level) && (0 != json_token_streq (json, t, "containerS")))
	    plugin_error (STATE_UNKNOWN, 0,
			  "json_parser: expected string \"containerS\" not found");
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
			  "json_parser: root element must be an object");

	}

      if (array_is_full (vals, keys_num))
	{
	  if (STREQ (*containerrunning, "true"))
	    counter_put (*ht_running, *image, 1);
	  else
	    counter_put (*ht_exited, *image, 1);
	  dbg ("new container found:\n");
	  for (size_t j = 0; j < keys_num; j++)
	    {
	      dbg (" * \"%s\": \"%s\"\n", keys[j], vals[j]);
	      vals[j] = NULL;
	    }
	}
    }

  free (tokens);
}

int
podman_running_containers (struct podman_varlink *pv, unsigned int *count,
			   const char *image, char **perfdata, bool verbose)
{
  const char *varlinkmethod = "io.podman.GetContainersByStatus";

  hashtable_t *ht_running, *ht_exited;
  char *errmsg = NULL, *json;
  unsigned int running_containers = 0, exited_containers = 0;

  json = podman_varlink_get (pv, varlinkmethod, &errmsg);
  if (NULL == json)
    {
      podman_varlink_unref (pv);
      plugin_error (STATE_UNKNOWN, 0, "%s", errmsg);
    }
  dbg ("varlink %s returned: %s", varlinkmethod, json);

  json_parser (json, &ht_running, &ht_exited);

  if (image)
    {
      hashable_t *np_exited = counter_lookup (ht_exited, image),
	*np_running = counter_lookup (ht_running, image);

      exited_containers = np_exited ? np_exited->count : 0;

      running_containers = np_running ? np_running->count : 0;
      *perfdata =
	xasprintf ("containers_exited_%s=%u containers_running_%s=%u", image,
		   exited_containers, image, running_containers);
    }
  else
    {
      exited_containers = counter_get_elements (ht_exited);
      running_containers = counter_get_elements (ht_running);

      size_t size;
      FILE *stream = open_memstream (perfdata, &size);
      for (unsigned int j = 0; j < ht_running->uniq; j++)
	{
	  hashable_t *np = counter_lookup (ht_running, ht_running->keys[j]);
	  assert (NULL != np);
	  fprintf (stream, "containers_%s=%lu ",
		   ht_running->keys[j], np->count);
	}
      fprintf (stream,
	       "containers_exited_total=%u containers_running_total=%u",
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
