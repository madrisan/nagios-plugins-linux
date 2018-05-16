/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking sysfs for Docker exposed metrics.
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

static const char *docker_socket = DOCKER_SOCKET;

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "common.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "system.h"
#include "url_encode.h"
#include "xasprintf.h"

#include "json.h"

static unsigned int containers;

typedef struct chunk
{
  char *memory;
  size_t size;
} chunk_t;

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
docker_init (CURL ** curl_handle, chunk_t * chunk)
{
  chunk->memory = malloc (1);	/* will be grown as needed by the realloc above */
  chunk->size = 0;		/* no data at this point */

  curl_global_init (CURL_GLOBAL_ALL);

  /* init the curl session */
  *curl_handle = curl_easy_init ();
  if (NULL == (*curl_handle))
    plugin_error (STATE_UNKNOWN, errno,
		  "cannot start a libcurl easy session");

  curl_easy_setopt (*curl_handle, CURLOPT_UNIX_SOCKET_PATH, docker_socket);
  dbg ("CURLOPT_UNIX_SOCKET_PATH is set to \"%s\"\n", docker_socket);

  /* send all data to this function */
  curl_easy_setopt (*curl_handle, CURLOPT_WRITEFUNCTION,
		    write_memory_callback);
  curl_easy_setopt (*curl_handle, CURLOPT_WRITEDATA, (void *) chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt (*curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
}

static CURLcode
docker_get (CURL * curl_handle, char *url)
{
  CURLcode res;

  curl_easy_setopt (curl_handle, CURLOPT_URL, url);
  res = curl_easy_perform (curl_handle);

  return res;
}

static void
docker_close (CURL * curl_handle, chunk_t * chunk)
{
  /* cleanup curl stuff */
  curl_easy_cleanup (curl_handle);

  free (chunk->memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup ();
}

static void process_json_value (json_value * value, int depth);

static void
process_json_object (json_value * value, int depth)
{
  int length, x;
  if (NULL == value)
    return;

  length = value->u.object.length;
  for (x = 0; x < length; x++)
    {
      if (json_string == (value->u.object.values[x].value)->type)
	{
	  if (STREQ ("Id", value->u.object.values[x].name))
	    {
	      dbg ("container id \"%s\"\n",
		   (value->u.object.values[x].value)->u.string.ptr);
	      containers++;
	    }
	}

      process_json_value (value->u.object.values[x].value, depth + 1);
    }
}

static void
process_json_array (json_value * value, int depth)
{
  int length, x;
  if (NULL == value)
    return;

  length = value->u.array.length;
  for (x = 0; x < length; x++)
    process_json_value (value->u.array.values[x], depth);
}

static void
process_json_value (json_value * value, int depth)
{
  if (NULL == value)
    return;

  switch (value->type)
    {
    default:
      break;
    case json_object:
      process_json_object (value, depth + 1);
      break;
    case json_array:
      process_json_array (value, depth + 1);
      break;
    }
}

/* Returns the number of running Docker containers  */
int
docker_running_containers_number (bool verbose)
{
  CURL *curl_handle = NULL;
  CURLcode res;
  chunk_t chunk;

  char *api_version = "1.18";
  char *encoded_filter = url_encode ("{\"status\":{\"running\":true}}");
  char *rest_url =
    xasprintf ("http://v%s/containers/json?filters=%s", api_version, encoded_filter);
  dbg ("rest encoded url: %s\n", rest_url);
  free (encoded_filter);

  docker_init (&curl_handle, &chunk);

  res = docker_get (curl_handle, rest_url);
  free (rest_url);

  if (CURLE_OK != res)
    {
      docker_close (curl_handle, &chunk);
      plugin_error (STATE_UNKNOWN, errno, "%s", curl_easy_strerror (res));
    }
  else
    {
      dbg ("%lu bytes retrieved\n", chunk.size);
      dbg ("json output: %s", chunk.memory);
    }

  /* parse the json stream returned by Docker */
  {
    json_char *json = chunk.memory;
    json_value *value;
    containers = 0;

    value = json_parse (json, chunk.size);
    if (NULL == value)
      {
	docker_close (curl_handle, &chunk);
	plugin_error (STATE_UNKNOWN, 0,
		      "unable to parse the json data returned by docker");
      }

    process_json_value (value, 0);

    json_value_free (value);
  }

  docker_close (curl_handle, &chunk);
  return containers;
}
