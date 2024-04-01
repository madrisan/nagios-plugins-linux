// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2020 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library containing some helper functions for parsing data in JSON format.
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
#include <stdlib.h>
#include <string.h>

#include "jsmn.h"
#include "json_helpers.h"
#include "logging.h"
#include "system.h"
#include "xalloc.h"

int
json_token_streq (const char *json, jsmntok_t *tok, const char *s)
{
  if (tok->type == JSMN_STRING && (int) strlen (s) == tok->end - tok->start &&
      strncmp (json + tok->start, s, tok->end - tok->start) == 0)
    return 0;

  return -1;
}

char *
json_token_tostr (char *json, jsmntok_t *t)
{
  return xsubstrdup (json + t->start, t->end - t->start);
}

jsmntok_t *
json_tokenise (const char *json, size_t *ntoken)
{
  jsmn_parser parser;
  int r, ret;

  assert (NULL != json);

  jsmn_init (&parser);
  r = jsmn_parse (&parser, json, strlen (json), NULL, 0);
  if (r < 0)
    return NULL;

  dbg ("number of json tokens: %d\n", r);
  jsmntok_t *tokens = xnmalloc (r, sizeof (jsmntok_t));

  jsmn_init (&parser);
  ret = jsmn_parse (&parser, json, strlen (json), tokens, r);
  assert (ret >= 0);

  *ntoken = r;
  return tokens;
}

static int
json_dump (const char *json, jsmntok_t *t, size_t count, int indent,
	   FILE *stream)
{
  int i, j, k;

  if (t->type == JSMN_PRIMITIVE)
    {
      fprintf (stream, "%.*s", t->end - t->start, json + t->start);
      if (t->size == 0)
	fprintf (stream, "\n");
      return 1;
    }
  else if (t->type == JSMN_STRING)
    {
      fprintf (stream, "%.*s", t->end - t->start, json + t->start);
      if (t->size == 0)
	fprintf (stream, "\n");
      return 1;
    }
  else if (t->type == JSMN_OBJECT)
    {
      j = 0;
      jsmntok_t *token;

      fprintf (stream, "\n");

      for (i = 0; i < t->size; i++)
	{
	  for (k = 0; k < indent; k++)
	    fprintf (stream, "  ");

	  token = t + 1 + j;
	  j += json_dump (json, token, count - j, indent + 1, stream);

	  /* get the value of the previous JSON label */
	  if (token->size > 0)
	    {
	      fprintf (stream, ": ");
	      j += json_dump (json, t + 1 + j, count - j, indent + 1, stream);
	    }
	}
      return j + 1;
    }
  else if (t->type == JSMN_ARRAY)
    {
      j = 0;
      fprintf (stream, "\n");

      for (i = 0; i < t->size; i++)
	{
	  for (k = 0; k < indent - 1; k++)
	    fprintf (stream, "  ");

	  fprintf (stream, "  - ");
	  j += json_dump (json, t + 1 + j, count - j, indent + 1, stream);
	}

      return j + 1;
    }

  return 0;
}

void
json_dump_pretty (const char *json, char **dump, int indent)
{
  size_t ntoken, size;
  FILE *stream = open_memstream (dump, &size);

  jsmntok_t *tok = json_tokenise (json, &ntoken);
  json_dump (json, tok, tok->size, indent, stream);

  fclose (stream);
}

int
json_search (const char *json, const char *path, char **value)
{
  char *p, *object, *label, *saveptr;
  size_t i, ntoken;
  bool found_label = false, found_object = false;

  dbg ("checking for %s in the json data...\n", path);
  jsmntok_t *tok = json_tokenise (json, &ntoken);

  p = xstrdup (path);
  object = strtok_r (p, ".", &saveptr);
  label = strtok_r (0, ".", &saveptr);

  if (object == NULL || label == NULL)
    return -1;

  dbg ("ckeckin for label \"%s\" in the object \"%s\"\n", label, object);

  /* FIXME: the search algorithm sucks :( */
  for (i = 0; i < ntoken; i++)
    {
      if (0 == json_token_streq (json, &tok[i], object))
	{
	  dbg ("found object \"%s\"\n", object);
	  found_object = true;
	}
      else if (found_object && (tok[i].type == JSMN_STRING)
	  && (0 == json_token_streq (json, &tok[i], label)))
	{
	  dbg ("found label \"%s\"\n", label);
	  found_label = true;
	  /* get the associated value */
	  size_t strsize = tok[i + 1].end - tok[i + 1].start;
	  *value = xmalloc (strsize + 1);
	  memcpy (*value, json + tok[i + 1].start, strsize);
	  break;
	}
    }
  if (found_label && found_object && NULL != *value)
    dbg ("found label \"%s\" in the object \"%s\" with value \"%s\"\n", label,
	 object, *value);

  return 0;
}
