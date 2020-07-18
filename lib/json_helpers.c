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
#include <string.h>

#include "jsmn.h"
#include "json_helpers.h"
#include "logging.h"
#include "xalloc.h"

int
json_token_streq (const char *json, jsmntok_t * tok, const char *s)
{
  if (tok->type == JSMN_STRING && (int) strlen (s) == tok->end - tok->start &&
      strncmp (json + tok->start, s, tok->end - tok->start) == 0)
    return 0;

  return -1;
}

char *
json_token_tostr (char *json, jsmntok_t * t)
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
