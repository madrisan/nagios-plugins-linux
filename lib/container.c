// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2018,2024 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking for Docker exposed metrics.
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
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBCURL
#include <curl/curl.h>
#endif

#include "common.h"
#include "collection.h"
#include "container.h"
#include "getenv.h"
#include "json_helpers.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "system.h"
#include "url_encode.h"
#include "xalloc.h"
#include "xasprintf.h"
#include "xstrton.h"

/* Hide all jsmn API symbols by making them static */
#define JSMN_STATIC
#include "jsmn.h"

#define DOCKER_CONTAINERS_JSON  0x01
#define DOCKER_STATS_JSON       0x02

/* returns the last portion of the given container image.
   example:
     "prom/prometheus:v2.39.0"  -->  "prometheus:v2.39.0"
   note that docker just removes the prefix "docker.io/".  */
static const char *
image_shortname (const char *image)
{
  const char *strip = strrchr (image, '/');
  if (NULL == strip)
    strip = image;
  else
    strip++;
  dbg ("shortening of \"%s\" to \"%s\"\n", image, strip);

  return strip;
}

/* parse the json stream returned by the Docker/Podman API and return a pointer
   to the hashtable containing the values of the discovered 'tokens'.
   return NULL if the data cannot be parsed.
   the function 'convert' allows a manipulation of the data before saving it in
   the hashtable.  */

static hashtable_t *
docker_json_parser_search (const char *json, const char *token,
			   const char *(*convert) (const char *),
			   unsigned long increment)
{
  size_t i, ntoken;
  hashtable_t *hashtable = NULL;

  jsmntok_t *buffer = json_tokenise (json, &ntoken);
  hashtable = counter_create ();

  for (i = 1; i < ntoken; i++)
    {
      if (0 == json_token_streq (json, &buffer[i], token))
	{
	  size_t strsize = buffer[i + 1].end - buffer[i + 1].start;
	  char *value = xmalloc (strsize + 1);
	  memcpy (value, json + buffer[i + 1].start, strsize);

	  dbg ("found token \"%s\" with value \"%s\" at position %d\n",
	       token, value, buffer[i].start);
	  counter_put (hashtable, convert ? convert (value) : value, increment);
	  free (value);
	}
    }

  free (buffer);
  return hashtable;
}

#if !defined NPL_TESTING && defined HAVE_LIBCURL

static size_t
write_memory_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  chunk_t *mem = (chunk_t *) userp;

  mem->memory = realloc (mem->memory, mem->size + realsize + 1);
  if (NULL == mem->memory)
    plugin_error (STATE_UNKNOWN, errno, "memory exhausted");

  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static void
docker_init (CURL **curl_handle, chunk_t *chunk, const char *socket)
{
  chunk->memory = malloc (1);	/* will be grown as needed by the realloc above */
  chunk->size = 0;		/* no data at this point */

  if (NULL == socket)
    {
      const char *env_docker_host = secure_getenv ("DOCKER_HOST");
      if (env_docker_host)
	{
	  dbg ("socket set using DOCKER_HOST (\"%s\")\n", env_docker_host);
	  socket = env_docker_host;
	}
      else
	plugin_error (STATE_UNKNOWN, 0,
		      "the socket path was not set, nor was the environment variable DOCKER_HOST");
    }

  curl_global_init (CURL_GLOBAL_ALL);

  /* init the curl session */
  *curl_handle = curl_easy_init ();
  if (NULL == (*curl_handle))
    plugin_error (STATE_UNKNOWN, errno,
		  "cannot start a libcurl easy session");

  curl_easy_setopt (*curl_handle, CURLOPT_UNIX_SOCKET_PATH, socket);
  dbg ("CURLOPT_UNIX_SOCKET_PATH is set to \"%s\"\n", socket);

  /* send all data to this function */
  curl_easy_setopt (*curl_handle, CURLOPT_WRITEFUNCTION,
		    write_memory_callback);
  curl_easy_setopt (*curl_handle, CURLOPT_WRITEDATA, (void *) chunk);
  curl_easy_setopt (*curl_handle, CURLOPT_NOPROGRESS, 1L);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt (*curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
}

static CURLcode
docker_get (CURL *curl_handle, const int query, const char *id)
{
  CURLcode res;
  char *api_version, *class, *url, *filter = NULL;

  api_version = secure_getenv ("DOCKER_API_VERSION");
  if (NULL == api_version)
    api_version = "1.24";  /* API versions before v1.24 are deprecated. */
  else
    dbg ("setting Docker API version according to DOCKER_API_VERSION: %s\n",
	 api_version);

  switch (query)
    {
    default:
      plugin_error (STATE_UNKNOWN, 0, "unknown docker query");
    case DOCKER_CONTAINERS_JSON:
      filter = url_encode ("{\"status\":{\"running\":true}}");
      class = xasprintf ("containers/json");
      url = filter ?
	xasprintf ("http://v%s/%s?filters=%s", api_version, class, filter) :
	xasprintf ("http://v%s/%s", api_version, class);
      break;
    case DOCKER_STATS_JSON:
      class = xasprintf ("containers/%s/stats", id);
      url = xasprintf ("http://v%s/%s?stream=false", api_version, class);
      break;
    }

  dbg ("docker rest url: %s\n", url);

  curl_easy_setopt (curl_handle, CURLOPT_URL, url);
  res = curl_easy_perform (curl_handle);

  free (class);
  free (filter);
  free (url);

  return res;
}

static void
docker_close (CURL *curl_handle, chunk_t *chunk)
{
  /* cleanup curl stuff */
  curl_easy_cleanup (curl_handle);

  free (chunk->memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup ();
}

#endif /* NPL_TESTING */

static int
docker_api_call (char *socket, chunk_t *chunk,
#ifndef NPL_TESTING
		 CURL *curl_handle,
#endif
		 const char *search_key, bool verbose)
{
#ifndef NPL_TESTING

  CURLcode res;

  curl_handle = NULL;
  docker_init (&curl_handle, chunk, socket);
  res = docker_get (curl_handle, DOCKER_CONTAINERS_JSON, NULL);
  if (CURLE_OK != res)
    {
      docker_close (curl_handle, chunk);
      plugin_error (STATE_UNKNOWN, errno, "%s", curl_easy_strerror (res));
    }

#else

  int err;
  if ((err = docker_get (chunk, DOCKER_CONTAINERS_JSON, NULL)) != 0)
    return err;

#endif /* NPL_TESTING */

  assert (chunk->memory);
  dbg ("%zu bytes retrieved\n", chunk->size);
  dbg ("json data: %s", chunk->memory);

#ifdef ENABLE_DEBUG
  char *dump;

  json_dump_pretty (chunk->memory, &dump, 1);
  dbg ("json data pretty formatted:%s", dump);
  free (dump);
#endif

  return 0;
}

/* Returns the number of running Docker/Podman containers grouped by images  */

int
docker_running_containers (char *socket, unsigned int *count,
			   const char *image, char **perfdata, bool verbose)
{
  chunk_t chunk;
#ifndef NPL_TESTING
  CURL *curl_handle = NULL;
#endif

  hashtable_t *hashtable;
  unsigned int running_containers = 0;
  char *search_key = "Image";

#ifndef NPL_TESTING
  docker_api_call (socket, &chunk, curl_handle, search_key, verbose);
#else
  docker_api_call (socket, &chunk, search_key, verbose);
#endif

  hashtable =
    docker_json_parser_search (chunk.memory, search_key, image_shortname, 1);
  if (NULL == hashtable)
    plugin_error (STATE_UNKNOWN, 0,
		  "unable to parse the json data for \"%s\"s", search_key);
  dbg ("number of docker unique containers (by %s): %u\n", search_key,
       counter_get_unique_elements (hashtable));

  if (image)
    {
      hashable_t *np = counter_lookup (hashtable, image);
      running_containers = np ? np->count : 0;
      *perfdata = xasprintf ("containers_%s=%u", image_shortname (image),
			     running_containers);
    }
  else
    {
      running_containers = counter_get_elements (hashtable);
      size_t size;
      FILE *stream = open_memstream (perfdata, &size);
      for (unsigned int j = 0; j < hashtable->uniq; j++)
	{
	  hashable_t *np = counter_lookup (hashtable, hashtable->keys[j]);
	  assert (NULL != np);
	  fprintf (stream, "containers_%s=%lu ",
		   image_shortname (hashtable->keys[j]), np->count);
	}
      fprintf (stream, "containers_total=%u", hashtable->elements);
      fclose (stream);
    }

  *count = running_containers;

  counter_free (hashtable);
  docker_close (
#ifndef NPL_TESTING
		 curl_handle,
#endif
		 &chunk);

  return 0;
}

#ifndef NPL_TESTING

/* Returns the memory usage of a Docker/Podman container  */
static int
docker_container_memory (char *socket, const char *id,
			 long long unsigned int *memory, bool verbose)
{
  chunk_t chunk;
  CURL *curl_handle = NULL;
  CURLcode res;
  char *errmesg = NULL;
  int ret;

  docker_init (&curl_handle, &chunk, socket);
  res = docker_get (curl_handle, DOCKER_STATS_JSON, id);
  if (CURLE_OK != res)
    {
      docker_close (curl_handle, &chunk);
      plugin_error (STATE_UNKNOWN, errno, "%s", curl_easy_strerror (res));
    }

  assert (chunk.memory);
  dbg ("%zu bytes retrieved\n", chunk.size);
  dbg ("json data: %s", chunk.memory);

#ifdef ENABLE_DEBUG
  /*
  char *dump;

  json_dump_pretty (chunk.memory, &dump, 1);
  dbg ("json data pretty formatted:%s", dump);
  free (dump);
  */
#endif

  /* get the memory usage  */
  char *value = NULL;
  json_search (chunk.memory, ".memory_stats.usage", &value);

  /* FIXME: remove cast  */
  ret = sizetollint (value, (long long int *)memory, &errmesg);
  if (ret < 0)
    plugin_error (STATE_UNKNOWN, errno
		  , "failed to convert container memory value: %s"
		  , errmesg);

  free (value);

  return 0;
}

/* Returns the memory usage of running Docker/Podman containers  */

int
docker_running_containers_memory (char *socket, long long unsigned int *memory,
				  bool verbose)
{
  chunk_t chunk;
  CURL *curl_handle = NULL;
  hashtable_t *hashtable;
  char *search_key = "Id";

  /* get the list of running containers  */
  docker_api_call (socket, &chunk, curl_handle, search_key, verbose);

  hashtable =
    docker_json_parser_search (chunk.memory, search_key, NULL, 1);
  if (NULL == hashtable)
    plugin_error (STATE_UNKNOWN, 0,
		  "unable to parse the json data for \"%s\"s", search_key);
  dbg ("number of docker containers: %u\n",
       counter_get_unique_elements (hashtable));

  long long unsigned int memory_usage;
  *memory = 0;

  /* FIXME: this loop must be threaded and run in parallel  */
  for (unsigned int j = 0; j < hashtable->uniq; j++)
    {
      const char *id = hashtable->keys[j];
      docker_container_memory (socket, id, &memory_usage, verbose);
      dbg ("memory usage for container id \"%s\": %llu\" bytes\n", id,
	   memory_usage);
      *memory += (memory_usage >> 10);
    }

  docker_close (
#ifndef NPL_TESTING
                 curl_handle,
#endif
                 &chunk);

  return 0;
}

#endif
