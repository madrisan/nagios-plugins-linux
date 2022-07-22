// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the Linux Pressure Stall Information (PSI)
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

#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "files.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "system.h"
#include "xalloc.h"
#include "xasprintf.h"

static int deep = 0;

static void
files_data_init (struct files_types **filecount)
{
  if (NULL == *filecount)
    {
      struct files_types *t;
      t = xmalloc (sizeof (struct files_types));
      *filecount = t;
    }
}

static int
files_filematch (const char *pattern, const char *name)
{
  if (pattern != NULL)
    {
      int status = fnmatch (pattern, name, /* flags = */ 0);
      if (status != 0)
	return FNM_NOMATCH;
    }

  return 0;
}

static bool
files_is_hidden (const char *filename)
{
  return (filename[0] == '.');
}

static bool
files_check_age (int64_t age, time_t now, time_t filemtime)
{
  time_t mtime = now;

  if (age == 0)
    return true;

  if (age < 0)
    mtime += age;
  else
    mtime -= age;

  return (((age < 0) && (filemtime > mtime)) ||
	  ((age > 0) && (filemtime < mtime)));
}

static bool
files_check_size (int64_t size, off_t filesize)
{
  off_t abs_size;

  if (size == 0)
    return true;

  if (size < 0)
    abs_size = (off_t) ((-1) * size);
  else
   abs_size = (off_t) size;

  return (((size < 0) && (filesize < abs_size)) ||
	  ((size > 0) && (filesize > abs_size)));
}

int
files_filecount (const char *dir, unsigned int flags,
		 int64_t age, int64_t size, const char *pattern,
		 struct files_types **filecount)
{
  int status;
  DIR *dirp;
  time_t now;

  errno = 0;
  if ((dirp = opendir (dir)) == NULL)
    {
      dbg ("(e) cannot open %s (%s)\n", dir, strerror (errno));
      return -1;
    }

  files_data_init (filecount);
  now = time (NULL);

  /* Scan entries under the 'dirp' directory */
  for (;;)
    {
      char abs_path[PATH_MAX];
      struct dirent *dp;
      struct stat statbuf;
      bool is_hidden, age_match, size_match;
      errno = 0;

      if ((dp = readdir (dirp)) == NULL)
	{
	  if (errno != 0)
	    plugin_error (STATE_UNKNOWN, errno, "readdir() failure");
	  else
	    break;		/* end-of-directory */
	}

      /* ignore directory entries */
      if (STREQ (dp->d_name, ".") || STREQ (dp->d_name, ".."))
	continue;

      is_hidden = files_is_hidden (dp->d_name);
      if (!(flags & FILES_INCLUDE_HIDDEN) && is_hidden)
	continue;

      snprintf (abs_path, sizeof (abs_path), "%s/%s", dir, dp->d_name);

      status = lstat (abs_path, &statbuf);
      if (status != 0)
	plugin_error (STATE_UNKNOWN, errno, "lstat (%s) failed", abs_path);

      if (S_IFDIR == (statbuf.st_mode & S_IFMT))
	{
	  dbg ("(%d) %s (%sdirectory)\n", deep, abs_path,
	       is_hidden ? "hidden " : "");
	  if (flags & FILES_RECURSIVE)
	    {
	     char *subdir = xasprintf ("%s/%s", dir, dp->d_name);
	      if (!(flags & FILES_REGULAR_ONLY))
		{
		  if (0 == files_filematch (pattern, dp->d_name))
		    {
		      (*filecount)->directory++;
		      (*filecount)->total++;
		      if (is_hidden)
			(*filecount)->hidden++;
		    }
		  dbg ("(%d)  --> #%lu\n", deep,
		       (unsigned long)(*filecount)->total);
		}

	      deep++;
	      dbg ("+ recursive call of files_filecount for %s\n", subdir);
	      files_filecount (subdir, flags, age, size,
			       pattern, filecount);
	      dbg ("(%d)  --> #%lu\n", deep,
		   (unsigned long)(*filecount)->total);
	      free (subdir);
	      continue;
	    }
	  if (flags & FILES_REGULAR_ONLY)
	    continue;
	}

      if (0 != files_filematch (pattern, dp->d_name))
	{
	  dbg ("(%d) %s does not match the pattern\n", deep, abs_path);
	  continue;
	}

      switch (statbuf.st_mode & S_IFMT)
	{
	default:
	  dbg ("(%d) %s (unknown file)\n", deep, abs_path);
	  (*filecount)->unknown++;
	  if (flags & FILES_IGNORE_UNKNOWN)
	    continue;
	  break;
	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
	case S_IFSOCK:
	  dbg ("(%d) %s (special file)\n", deep, abs_path);
	  (*filecount)->special_file++;
	  if (flags & FILES_REGULAR_ONLY)
	    continue;
	  break;
	case S_IFLNK:
	  dbg ("(%d) %s (symlink)\n", deep, abs_path);
	  if (flags & (FILES_IGNORE_SYMLINKS | FILES_REGULAR_ONLY))
	    continue;
	  (*filecount)->symlink++;
	  break;
	case S_IFREG:
	  age_match = files_check_age (age, now, statbuf.st_mtime);
	  size_match = files_check_size (size, statbuf.st_size);
	  dbg ("(%d) %s (%s file), touched %.2f days ago (%s)"
	       " with size %lu bytes (%s)\n"
	       , deep
	       , abs_path
	       , is_hidden ? "hidden" : "regular"
	       , (long)(now - statbuf.st_mtime) / 60.0 / 60.0 / 24.0
	       , age_match ? "match" : "skip"
	       , (unsigned long) statbuf.st_size
	       , size_match ? "match" : "skip");
	  if (!age_match)
	    continue;
	  if (!size_match)
	    continue;

	  (*filecount)->regular_file++;
	  if (is_hidden)
	    (*filecount)->hidden++;
	  break;
	}

      if (files_filematch (pattern, dp->d_name) != 0)
	continue;

      (*filecount)->total++;
      dbg ("(%d)  --> #%lu\n", deep, (unsigned long)(*filecount)->total);
    }

  dbg ("- return #%lu (%s)\n", (unsigned long)(*filecount)->total, dir);
  deep--;
  closedir (dirp);
  return 0;
}
