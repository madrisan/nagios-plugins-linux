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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "messages.h"
#include "procparser.h"

static char buf[2048];

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
  char namebuf[16];		/* big enough to hold any row name */
  proc_table_struct findme = { namebuf, NULL };
  proc_table_struct *found;
  char *head;
  char *tail;
  int fd, local_n;

#if __SIZEOF_LONG__ == 4
  unsigned long long slotll;
#endif

  if ((fd = open (filename, O_RDONLY)) == -1)
    plugin_error (STATE_UNKNOWN, 0, "Error: /proc must be mounted");
  if ((local_n = read (fd, buf, sizeof buf - 1)) < 0)
    plugin_error (STATE_UNKNOWN, 0, "Error reading %s", filename);
  buf[local_n] = '\0';
  close (fd);

  head = buf;
  for (;;)
    {
      tail = strchr (head, separator);
      if (!tail)
	break;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
	{
	  head = tail + 1;
	  goto nextline;
	}
      strcpy (namebuf, head);
      found = bsearch (&findme, proc_table, proc_table_count,
		       sizeof (proc_table_struct),
		       compare_proc_table_structs);
      head = tail + 1;
      if (!found)
	goto nextline;

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

    nextline:
      tail = strchr (head, '\n');
      if (!tail)
	break;
      head = tail + 1;
    }
}
