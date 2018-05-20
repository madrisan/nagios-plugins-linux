/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
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
  hashtable->elements = 0;
  hashtable->table = table;

  return hashtable;
}

/* lookup: look for s in hash table */
hashable_t *
counter_lookup (const hashtable_t * hashtable, const char *s)
{
  hashable_t *np;

  for (np = hashtable->table[hash (s)]; np != NULL; np = np->next)
    if (strcmp (s, np->key) == 0)
      return np;		/* found */

  return NULL;			/* not found */
}

/* Insert the element 'key' into the hash table.
 * Set count to zero if 'key' was not present in the table or
 * increment the counter otherwise.  */
hashable_t *
counter_put (hashtable_t * hashtable, const char *key)
{
  hashable_t *np;
  unsigned hashval;

  if ((np = counter_lookup (hashtable, key)) == NULL)
    {				/* not found */
      np = xmalloc (sizeof (*np));
      if (NULL == (np->key = strdup (key)))
	return NULL;
      np->count = 1;
      hashval = hash (key);
      np->next = hashtable->table[hashval];
      hashtable->table[hashval] = np;
    }
  else
    /* already there */
    np->count++;

  hashtable->elements++;

  return np;
}

unsigned int
counter_get_elements (const hashtable_t * hashtable)
{
  return hashtable->elements;
}

void
counter_free (hashtable_t * hashtable)
{
  hashable_t *np, *np2;
  int i = 0;

  for (i = 0; i < HASHSIZE; i++)
    {
      np = hashtable->table[i];
      while (np != NULL)
	{
	  np2 = np;
	  np = np->next;
	  free (np2);
	}
      free (hashtable->table[i]);
    }
  free (hashtable->table);
  free (hashtable);
}
