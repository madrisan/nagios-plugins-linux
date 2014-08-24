/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A parser for the /proc files /proc/meminfo and /proc/vmstat
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "messages.h"
#include "procparser.h"
#include "xalloc.h"

static int
compare_proc_table_structs (const void *a, const void *b)
{
  return strcmp (((const proc_table_struct *) a)->name,
		 ((const proc_table_struct *) b)->name);
}

void
procparser (char *filename, const proc_table_struct *proc_table,
	    int proc_table_count, char separator)
{
  char namebuf[32];		/* big enough to hold any row name */
  proc_table_struct findme = { namebuf, NULL };
  proc_table_struct *found;
  char *line = NULL, *head, *tail;
  FILE *fp;
  size_t len = 0;
  ssize_t chread;

#if __SIZEOF_LONG__ == 4
  unsigned long long slotll;
#endif

  if ((fp = fopen (filename,  "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error: cannot read %s", filename);

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      head = line;
      tail = strchr (line, separator);
      if (!tail)
	continue;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
	continue;
      strcpy (namebuf, head);
      found = bsearch (&findme, proc_table, proc_table_count,
		       sizeof (proc_table_struct),
		       compare_proc_table_structs);
      head = tail + 1;
      if (!found)
	continue;

#if __SIZEOF_LONG__ == 4
      /* A 32 bit kernel would have already truncated the value, a 64 bit kernel
       * doesn't need to.  Truncate here to let 32 bit programs to continue to get
       * truncated values.  It's that or change the API for a larger data type.
       */
      slotll = strtoull (head, &tail, 10);
      *(found->slot) = (unsigned long) slotll;
#else
      *(found->slot) = strtoul (head, &tail, 10);
#endif
    }

  free (line);
}

int
linelookup (char *line, char *pattern, char **value)
{
  char *p, *v;
  int len = strlen (pattern);

  if (!*line)
    return 0;

  /* pattern */
  if (strncmp (line, pattern, len))
    return 0;

  /* white spaces */
  for (p = line + len; isspace (*p); p++);

  /* separator */
  if (*p != ':')
    return 0;

  /* white spaces */
  for (++p; isspace (*p); p++);

  /* value */
  if (!*p)
    return 0;
  v = p;

  /* end of value */
  len = strlen (line) - 1;
  for (p = line + len; isspace (*(p - 1)); p--);
  *p = '\0';

  *value = xstrdup (v);
  return 1;
}
