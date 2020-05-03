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
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <varlink.h>

#include "common.h"
#include "container_podman.h"
#include "logging.h"
#include "messages.h"
#include "xalloc.h"
#include "xasprintf.h"

static inline int
epoll_control (int epfd, int op, int fd, uint32_t events, void *ptr)
{
  struct epoll_event event = {
    .data = {.ptr = ptr},
    .events = events
  };

  return epoll_ctl (epfd, op, fd, &event);
}

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
podman_varlink_check_event (podman_varlink_t *pv)
{
  int ret, timeout = -1;
  struct epoll_event event;

  dbg ("executing varlink_connection_get_events...\n");
  if ((ret = varlink_connection_get_events (pv->connection)) < 0)
    return ret;

  dbg ("executing epoll_control...\n");
  ret = epoll_control (pv->epoll_fd, EPOLL_CTL_ADD,
		       varlink_connection_get_fd (pv->connection),
		       varlink_connection_get_events (pv->connection),
		       pv->connection);
  if (ret < 0 && errno != EEXIST)
    return ret;

  dbg ("executing epoll wait loop...\n");
  for (;;)
    {
      ret = epoll_wait (pv->epoll_fd, &event, 1, timeout);
      if (ret < 0)
	{
	  if (EINTR == errno)
	    continue;
	  return -errno;
	}
      if (ret == 0)
	return -ETIMEDOUT;

      if (event.data.ptr == pv->connection)
	{
	  epoll_control (pv->epoll_fd, EPOLL_CTL_MOD,
			 varlink_connection_get_fd (pv->connection),
			 varlink_connection_get_events (pv->connection),
			 pv->connection);
	  break;	/* ready */
	}
      if (NULL == event.data.ptr)
	{
	  struct signalfd_siginfo info;
	  long size;

	  size = read (pv->signal_fd, &info, sizeof (info));
	  if (size != sizeof (info))
	    continue;
	  return -EINTR;
	}
    }

  dbg ("processing varlink pending events...\n");
  if ((ret = varlink_connection_process_events (pv->connection, 0)) != 0)
    return ret;

  return 0;
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
podman_varlink_new (podman_varlink_t **pv, char *varlinkaddr)
{
  long ret;
  sigset_t mask;
  podman_varlink_t *v;

  if (NULL == varlinkaddr)
    varlinkaddr = VARLINK_ADDRESS;
  dbg ("varlink address is \"%s\"\n", varlinkaddr);

  dbg ("allocating memory for the \"podman_varlink_t\" structure...\n");
  v = calloc (1, sizeof (podman_varlink_t));
  if (!v)
    plugin_error (STATE_UNKNOWN, 0, "podman_varlink_new: memory exhausted");

  v->parameters = NULL;

  dbg ("opening an epoll file descriptor and set the close-on-exec flag...\n");
  if ((v->epoll_fd = epoll_create1 (EPOLL_CLOEXEC)) < 0)
    plugin_error (STATE_UNKNOWN, errno, "epoll_create1: %s (errno: %d)",
		  strerror (errno), errno);

  dbg ("setting POSIX signals...\n");
  sigemptyset (&mask);
  sigaddset (&mask, SIGTERM);
  sigaddset (&mask, SIGINT);
  sigaddset (&mask, SIGPIPE);
  sigprocmask (SIG_BLOCK, &mask, NULL);

  dbg ("creating a file descriptor for accepting signals...\n");
  v->signal_fd = signalfd (-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (v->signal_fd < 0)
    plugin_error (STATE_UNKNOWN, errno, "signalfd: %s (errno: %d)",
		  strerror (errno), errno);

  dbg ("add signal_fd to the interest list of the epoll...\n");
  epoll_control (v->epoll_fd, EPOLL_CTL_ADD, v->signal_fd, EPOLLIN, NULL);

  dbg ("create a new varlink client connection...\n");
  ret = varlink_connection_new (&v->connection, varlinkaddr);
  if (ret < 0)
    plugin_error (STATE_UNKNOWN, errno, "varlink_connection_new: %s",
		  varlink_error_string (labs (ret)));

  *pv = v;
  dbg ("the varlink resources have been successfully initialized\n");
  return 0;
}

podman_varlink_t *
podman_varlink_unref (podman_varlink_t *pv)
{
  if (pv == NULL)
    return NULL;

  epoll_control (pv->epoll_fd, EPOLL_CTL_DEL,
		 varlink_connection_get_fd (pv->connection), 0, NULL);

  varlink_connection_close (pv->connection);
  varlink_connection_free (pv->connection);
  pv->connection = NULL;

  if (pv->parameters)
    varlink_object_unref (pv->parameters);
  free (pv);

  return NULL;
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

  assert (NULL != pv->connection);

  dbg ("executing varlink_connection_call with method set to \"%s\"...\n",
       varlinkmethod);
  ret = varlink_connection_call (pv->connection, varlinkmethod, pv->parameters,
				 0, podman_varlink_callback, &reply);
  if (ret < 0)
    return podman_varlink_error (ret, "varlink_connection_call", err);

  if ((ret = podman_varlink_check_event (pv)) < 0)
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

  assert (NULL != pv->connection);

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

  if ((ret = podman_varlink_check_event (pv)) < 0)
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
