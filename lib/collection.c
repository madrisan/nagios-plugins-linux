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
hash (char *s)
{
  unsigned hashval;
  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;

  return hashval % HASHSIZE;
}

/* initialize the hash pointer table */
hashable_t **
counter_init (hashable_t **hashtable[])
{
  *hashtable = xnmalloc (HASHSIZE, sizeof (hashable_t *));
  return *hashtable;
}

/* lookup: look for s in hashtable */
hashable_t *
counter_lookup (hashable_t *hashtable[], char *s)
{
  hashable_t *np;
  for (np = hashtable[hash (s)]; np != NULL; np = np->next)
    if (strcmp (s, np->key) == 0)
      return np;                /* found */

  return NULL;                  /* not found */
}

/* Insert the element 'key' into the hash table.
 * Set count to zero if 'key' was not present in the table or
 * increment the counter otherwise.  */
hashable_t *
counter_put (hashable_t *hashtable[], char *key)
{
  hashable_t *np;
  unsigned hashval;

  if ((np = counter_lookup (hashtable, key)) == NULL)
    {                           /* not found */
      np = (hashable_t *) malloc (sizeof (*np));
      /* FIXME: use npl-native allocation functions instead */
      if (np == NULL || (np->key = strdup (key)) == NULL)
	return NULL;
      np->count = 1;
      hashval = hash (key);
      np->next = hashtable[hashval];
      hashtable[hashval] = np;
    }
  else
    /* already there */
    np->count++;

  return np;
}

void
counter_free (hashable_t *hashtable[])
{
  hashable_t *np;
  int i = 0;

  for (i = 0; i < HASHSIZE; i++)
    {
      np = hashtable[i];
      free (np);
    } 
  free (hashtable);
}
