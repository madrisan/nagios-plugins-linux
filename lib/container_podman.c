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
#define CONTAINER_PODMAN_PRIVATE

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#ifndef NPL_TESTING
# include <varlink.h>
#endif
#include <string.h>
#include <sys/epoll.h>

#include "common.h"
#include "container_podman.h"
#include "messages.h"
#include "xalloc.h"
#include "xasprintf.h"

#undef CONTAINER_PODMAN_PRIVATE

#ifndef NPL_TESTING

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

int
podman_varlink_get (struct podman_varlink *pv, const char *varlinkmethod,
		    char *param, char **json, char **err)
{
  long ret;
  VarlinkObject *out;

  assert (pv->connection);
  if (NULL == param)
    ret = varlink_object_new (&pv->parameters);
  else
    ret = varlink_object_new_from_json (&pv->parameters, param);
  if (ret < 0)
    {
      *err =
	xasprintf ("varlink_object_new: %s",
		   varlink_error_string (labs (ret)));
      return ret;
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
