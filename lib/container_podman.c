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
#include <string.h>
#include <sys/epoll.h>
#include <varlink.h>

#include "common.h"
#include "container_podman.h"
#include "logging.h"
#include "messages.h"
#include "xalloc.h"
#include "xasprintf.h"

#ifndef NPL_TESTING

long
podman_varlink_error (long ret, const char *funcname, char **err)
{
  if (ret < 0)
    {
      const char *varlink_err = varlink_error_string (labs (ret));
      *err =
	funcname ? xasprintf ("%s: %s", funcname,
			      varlink_err) : xasprintf ("%s", varlink_err);
      return ret;
    }
  return 0;
}

long
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

long
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

long
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    char **json, char **err)
{
  long ret;
  VarlinkObject *out;

  assert (pv->connection);
  /*assert (pv->parameters);*/

  ret =
    varlink_connection_call (pv->connection, varlinkmethod, pv->parameters, 0,
			     podman_varlink_callback, &out);
  if (ret < 0)
    {
      *err =
	xasprintf ("varlink_connection_call: %s",
		   varlink_error_string (labs (ret)));
      return ret;
    }

  ret = podman_varlink_check_event (pv->connection, err);
  if (ret < 0)
    return ret;

  ret = varlink_object_to_json (out, json);
  if (ret < 0)
    {
      varlink_object_unref (out);
      return ret;
    }

  return 0;
}

/* Return the list of podman containers.
 * The format of the data in JSON format follows:
 *
 *     {
 *       "containers": [
 *         {
 *           "command": [
 *             ...
 *           ],
 *           "containerrunning": true,
 *           "createdat": "2020-04-06T00:22:29+02:00",
 *           "id": "3b395e067a3071ba77b2b44999235589a0957c72e99d1186ff1e53a0069da727",
 *           "image": "docker.io/library/redis:latest",
 *           "imageid": "f0453552d7f26fc38ffc05fa034aa7a7bc6fbb01bc7bc5a9e4b3c0ab87068627",
 *           "mounts": [
 *              ...
 *           ],
 *           "names": "srv-redis-1",
 *           "namespaces": {
 *             ...
 *           },
 *           "ports": [
 *             ...
 *           ],
 *           "rootfssize": 98203942,
 *           "runningfor": "52.347336247s",
 *           "rwsize": 0,
 *           "status": "running"
 *         },
 *         {
 *           ...
 *         }
 *       ]
 *     }
 */

long
podman_varlink_list (podman_varlink_t *pv, VarlinkArray **list, char **err)
{
  const char *varlinkmethod = "io.podman.ListContainers";
  long ret;
  VarlinkObject *reply;

  dbg ("varlink method \"%s\"\n", varlinkmethod);
  ret = varlink_connection_call (pv->connection, varlinkmethod,
				 pv->parameters, 0, podman_varlink_callback,
				 &reply);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_connection_call", err);

  ret = podman_varlink_check_event (pv->connection, err);
  if (ret < 0)
    return podman_varlink_error (ret, NULL, err);

  ret = varlink_object_get_array (reply, "containers", list);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_object_get_array", err);

  return 0;
}

/* Get the statistics for a running container.
 * The format of the data follows:
 *
 *     {
 *       "container": {
 *         "block_input": 16601088,
 *         "block_output": 16384,
 *         "cpu": 1.0191267811342149e-07,
 *         "cpu_nano": 1616621000,
 *         "id": "e15712d1db8f92b3a00be9649345856da53ab22abcc8b22e286c5cd8fbf08c36",
 *         "mem_limit": 8232525824,
 *         "mem_perc": 0.10095059739468847,
 *         "mem_usage": 8310784,
 *         "name": "srv-redis-1",
 *         "net_input": 1048,
 *         "net_output": 7074,
 *         "pids": 4,
 *         "system_nano": 1586280558931850500
 *      }
 */

long
podman_varlink_stats (podman_varlink_t *pv, const char *shortid,
		      container_stats_t *cp, char **err)
{
  const char *temp;
  char *varlinkmethod = "io.podman.GetContainerStats";
  long ret;
  VarlinkObject *reply, *stats;

  assert (pv->connection);

  dbg ("%s: parameter built from \"%s\" will be passed to varlink_call()\n",
       __func__, shortid);
  varlink_object_new (&(pv->parameters));
  ret = varlink_object_set_string (pv->parameters, "name", shortid);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_object_set_string", err);

  dbg ("varlink method \"%s\"\n", varlinkmethod);
  ret = varlink_connection_call (pv->connection, varlinkmethod,
				 pv->parameters, 0, podman_varlink_callback,
				 &reply);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_connection_call", err);

  varlink_object_unref (pv->parameters);

  ret = podman_varlink_check_event (pv->connection, err);
  if (ret < 0)
    return podman_varlink_error (ret, NULL, err);

  ret = varlink_object_get_object (reply, "container", &stats);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_object_get_object", err);

  varlink_object_get_int (stats, "block_input", &cp->block_input);
  varlink_object_get_int (stats, "block_output", &cp->block_output);
  varlink_object_get_int (stats, "net_input", &cp->net_input);
  varlink_object_get_int (stats, "net_output", &cp->net_output);
  varlink_object_get_float (stats, "cpu", &cp->cpu);
  varlink_object_get_int (stats, "mem_usage", &cp->mem_usage);
  varlink_object_get_int (stats, "mem_limit", &cp->mem_limit);
  varlink_object_get_int (stats, "pids", &cp->pids);
  varlink_object_get_string (stats, "name", &temp);
  cp->name = xstrdup (temp);

  dbg ("%s: block I/O = %ld/%ld\n", __func__, cp->block_input,
       cp->block_output);
  dbg ("%s: cpu = %.2lf%%\n", __func__, cp->cpu);
  dbg ("%s: network I/O = %ld/%ld\n", __func__, cp->net_input, cp->net_output);
  dbg ("%s: memory usage = %ld/%ld\n", __func__, cp->mem_usage, cp->mem_limit);
  dbg ("%s: pids = %ld\n", __func__, cp->pids);
  dbg ("%s: name = %s\n", __func__, cp->name);

  varlink_object_unref (reply);
  return 0;
}

#endif		/* NPL_TESTING */

/* Return true if the array has all the element non NULL, false otherwise */

bool
podman_array_is_full (char *v[], size_t vsize)
{
  for (size_t i = 0; i < vsize && v; i++)
    {
      // dbg ("v[%lu] = %s\n", i, v[i]);
      if (NULL == v[i])
	return false;
    }
  return true;
}

/* Return a string valid for Nagios performance data output */

char*
podman_image_name_normalize (const char *image)
{
  char *nstring = xstrdup (basename (image));
  for (size_t i = 0; i < strlen (nstring); i++)
    if (nstring[i] == ':')
      nstring[i] = '_';
  return nstring;
}

/* Return the short ID in the 'shortid' buffer. This buffer must have a size
   PODMAN_SHORTID_LEN   */

void
podman_shortid (const char *id, char *shortid)
{
  memcpy (shortid, id, PODMAN_SHORTID_LEN - 1);
  shortid[PODMAN_SHORTID_LEN - 1] = '\0';
}
