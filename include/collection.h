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
    unsigned int count;
  } hashable_t;

  typedef struct hashtable
  {
    unsigned int capacity;	/* hashtable capacity (in terms of hashed keys) */
    unsigned int elements;	/* number of elements stored in the hashtable */
    hashable_t **table;
  } hashtable_t;

  hashtable_t *counter_create (void);
  void counter_free (hashtable_t * hashtable);

  hashable_t *counter_lookup (const hashtable_t * hashtable, const char *s);
  hashable_t *counter_put (hashtable_t * hashtable, const char *key);
  unsigned int counter_get_elements (const hashtable_t * hashtable);

#ifdef __cplusplus
}
#endif

#endif				/* _COLLECTION_H_ */
