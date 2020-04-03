/*  collection.h -- dictionary for counting hashable objects (strings)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _COLLECTION_H_
#define _COLLECTION_H_

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct hashable
  {				/* table entry: */
    struct hashable *next;	/* next element in case of a collision */
    char *key;
    unsigned long count;
  } hashable_t;

  typedef struct hashtable
  {
    unsigned int capacity;	/* hashtable capacity */
    unsigned int elements;	/* number of elements stored */
    unsigned int uniq;		/* number of unique keys */
    hashable_t **table;
    char **keys;		/* array containing the ptrs to the keys */
  } hashtable_t;

  hashtable_t *counter_create (void);
  void counter_free (hashtable_t * hashtable);

  hashable_t *counter_lookup (const hashtable_t * hashtable, const char *key);
  hashable_t *counter_put (hashtable_t * hashtable, const char *key,
			   unsigned long increment);
  unsigned int counter_get_elements (const hashtable_t * hashtable);
  unsigned int counter_get_unique_elements (const hashtable_t * hashtable);
  char **counter_keys (hashtable_t * hashtable);

#ifdef __cplusplus
}
#endif

#endif				/* _COLLECTION_H_ */
