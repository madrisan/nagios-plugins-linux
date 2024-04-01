// SPDX-License-Identifier: GPL-3.0-or-later
/* json_helpers.h -- Helper function for parsing data in JSON format

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef JSON_HELPERS_H_
#define JSON_HELPERS_H_

#define JSMN_HEADER
#include "jsmn.h"

#ifdef __cplusplus
extern "C"
{
#endif

  int json_token_streq (const char *json, jsmntok_t * tok, const char *s);
  char *json_token_tostr (char *json, jsmntok_t * t);
  jsmntok_t *json_tokenise (const char *json, size_t *ntoken);
  void json_dump_pretty (const char *json, char **dump, int indent);
  int json_search (const char *json, const char *path, char **value);

#ifdef __cplusplus
}
#endif

#endif				/* JSON_HELPERS_H_ */
