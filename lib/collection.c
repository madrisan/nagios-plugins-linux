// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2018 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A simple dictionary for counting hashable objects (strings)
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

#include <stdlib.h>
#include <string.h>

#include "collection.h"
#include "logging.h"
#include "string-macros.h"
#include "xalloc.h"

#define HASHSIZE 101

/* hash: form hash value for string s */

static unsigned
hash (const char *s)
{
  unsigned hashval;

  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;

  return hashval % HASHSIZE;
}

/* initialize the hash pointer table */

hashtable_t *
counter_create (void)
{
  hashtable_t *hashtable = xmalloc (sizeof (hashtable_t));
  hashable_t **table = xnmalloc (HASHSIZE, sizeof (hashable_t *));

  hashtable->capacity = HASHSIZE;
  hashtable->elements = hashtable->uniq = 0;
  hashtable->table = table;
  hashtable->keys = xmalloc (sizeof (*(hashtable->keys)));

  return hashtable;
}

/* lookup: look for a key in hash table */

hashable_t *
counter_lookup (const hashtable_t * hashtable, const char *key)
{
  hashable_t *np;

  dbg ("hashtable lookup for key \"%s\"\n", key);
  for (np = hashtable->table[hash (key)]; np != NULL; np = np->next)
    if (STREQ (key, np->key))
      {
	dbg ("hashtable lookup: found key \"%s\"\n", key);
	return np;
      }

  dbg ("hashtable lookup: key \"%s\" not found\n", key);
  return NULL;
}

/* Insert the element 'key' into the hash table.
 * Set count to 'increment' if 'key' was not present in the table or
 * increment the counter by 'increment' otherwise.  */

hashable_t *
counter_put (hashtable_t * hashtable, const char *key, unsigned long increment)
{
  hashable_t *np;

  if ((np = counter_lookup (hashtable, key)) == NULL)
    {				/* not found */
      dbg ("hashtable: adding a new key \"%s\"\n", key);
      np = xmalloc (sizeof (*np));
      np->key = xstrdup (key);

      unsigned int hashval = hash (key);
      np->count = increment;
      np->next = hashtable->table[hashval];
      hashtable->table[hashval] = np;

      unsigned long new_size =
	sizeof (*(hashtable->keys)) * (hashtable->uniq + 1);
      hashtable->keys = xrealloc (hashtable->keys, new_size);
      dbg ("xrealloc'ed hashtable->keys to %lu bytes\n", new_size);

      (hashtable->keys)[hashtable->uniq] = np->key;
      dbg ("(hashtable->keys)[%u] = \"%s\"\n", hashtable->uniq,
	   (hashtable->keys)[hashtable->uniq]);

      hashtable->uniq++;
    }
  else
    {
      /* already there */
      dbg ("hashtable: the key \"%s\" is already present, "
	   "counter incremented: %lu\n", key, np->count);
      np->count += increment;
    }

  hashtable->elements++;

  return np;
}

/* Return the number of elements stored in the hash table */

unsigned int
counter_get_elements (const hashtable_t * hashtable)
{
  return hashtable->elements;
}

/* Return the number if unique elements stored in the hash table */

unsigned int
counter_get_unique_elements (const hashtable_t * hashtable)
{
  return hashtable->uniq;
}

/* Return an array containing the pointers to all the keys stored
   in the hash table */

char **
counter_keys (hashtable_t * hashtable)
{
  return (hashtable->uniq < 1) ? NULL : hashtable->keys;
}

void
counter_free (hashtable_t * hashtable)
{
  hashable_t *np, *np2;

  for (int i = 0; i < HASHSIZE; i++)
    {
      np = hashtable->table[i];
      while (np != NULL)
	{
	  np2 = np;
	  np = np->next;
	  free (np2);
	}
    }
  free (hashtable->table);
  free (hashtable->keys);
  free (hashtable);
}
