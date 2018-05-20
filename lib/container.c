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
#include "collection.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "system.h"
#include "url_encode.h"
#include "xalloc.h"
#include "xasprintf.h"

#include "json.h"

typedef struct chunk
{
  char *memory;
  size_t size;
} chunk_t;

typedef struct container
{
  char *image;		/* image name */
  unsigned int count;	/* number of container images */
} container_t;

static int
json_eq (const char *json, jsmntok_t * tok, const char *s)
{
  if (tok->type == JSMN_STRING && (int) strlen (s) == tok->end - tok->start &&
      strncmp (json + tok->start, s, tok->end - tok->start) == 0)
    return 0;

  return -1;
}

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

/* Returns the number of running Docker containers  */
unsigned int
docker_running_containers_number ()
{
  CURL *curl_handle = NULL;
  CURLcode res;
  chunk_t chunk;
  unsigned int running_containers = 0;

  char *api_version = "1.18";
  char *encoded_filter = url_encode ("{\"status\":{\"running\":true}}");
  char *rest_url =
    xasprintf ("http://v%s/containers/json?filters=%s", api_version,
	       encoded_filter);
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
    char *json = chunk.memory;
    jsmn_parser parser;
    int i, r;

    jsmn_init (&parser);
    r = jsmn_parse (&parser, json, strlen (json), NULL, 0);
    if (r < 0)
      plugin_error (STATE_UNKNOWN, 0,
		    "unable to parse the json data returned by docker");

    jsmntok_t *buffer = xnmalloc (r, sizeof (jsmntok_t));
    dbg ("number of json tokens: %d\n", r);

    jsmn_init (&parser);
    jsmn_parse (&parser, json, strlen (json), buffer, r);

    hashtable_t *hashtable = counter_create ();

    for (i = 1; i < r; i++)
      {
	if (json_eq (json, &buffer[i], "Id") == 0)
	  {
	    dbg ("found docker container with id \"%.*s\"\n",
		 buffer[i + 1].end - buffer[i + 1].start,
		 json + buffer[i + 1].start);
	    running_containers++;
	  }
	else if (json_eq (json, &buffer[i], "Image") == 0)
	  {
	    size_t strsize = buffer[i + 1].end - buffer[i + 1].start;
	    char *image = xmalloc (strsize + 1);
            memcpy (image, json + buffer[i + 1].start, strsize);

            dbg ("docker image \"%s\"\n", image);
            counter_put (hashtable, image);
	  }
      }

    // TODO: hashtable now contains the occurrences of
    //       docker images; return this data in some way...

    running_containers = counter_get_elements (hashtable);

    counter_free (hashtable);
    free (buffer);
  }

  docker_close (curl_handle, &chunk);
  return running_containers;
}
