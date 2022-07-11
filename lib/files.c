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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "files.h"
#include "logging.h"
#include "messages.h"
#include "string-macros.h"
#include "xalloc.h"
#include "xasprintf.h"

static int deep = 0;

int
files_filecount (const char *folder, unsigned int flags)
{
  int filecount = 0;
  DIR *dirp;

  errno = 0;
  if ((dirp = opendir (folder)) == NULL)
    {
      dbg ("(e) cannot open %s (%s)\n", folder, strerror (errno));
      return -1;
    }

  /* Scan entries under the 'dirp' directory */
  for (;;)
    {
      struct dirent *dp;
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

      if (!(flags & READDIR_INCLUDE_HIDDEN) && (dp->d_name[0] == '.'))
	continue;

      switch (dp->d_type)
	{
	default:
	  dbg ("(%d) %s/%s (other)\n", deep, folder, dp->d_name);
	  if (flags & READDIR_REGULAR_FILES_ONLY)
	    continue;
	  break;
	case DT_DIR:
	  dbg ("(%d) %s/%s (directory)\n", deep, folder, dp->d_name);
	  if (flags & READDIR_RECURSIVE)
	    {
	      char *subdir = xasprintf ("%s/%s", folder, dp->d_name);
	      if (!(flags & READDIR_REGULAR_FILES_ONLY))
		{
		  filecount++;
		  dbg ("(%d)  --> #%d\n", deep, filecount);
		}

	      deep++;
	      dbg ("+ recursive call of files_filecount for %s\n", subdir);
	      int partial = files_filecount (subdir, flags);
	      if (partial > 0)
		{
		  filecount += partial;
		  dbg ("(%d)  --> #%d\n", deep, filecount);
		}

	      free (subdir);
	      continue;
	    }
	  if (flags & READDIR_REGULAR_FILES_ONLY)
	    continue;
	  break;
	case DT_LNK:
	  dbg ("(%d) %s/%s (symlink)\n", deep, folder, dp->d_name);
	  if (flags & (READDIR_IGNORE_SYMLINKS | READDIR_REGULAR_FILES_ONLY))
	    continue;
	  break;
	case DT_REG:
	  dbg ("(%d) %s/%s (regular file)\n", deep, folder, dp->d_name);
	  break;
	case DT_UNKNOWN:
	  /* NOTE: according to the readdir manpage, only some filesystems
	   * (among them: Btrfs, ext2, ext3, and ext4) have full support for
	   * returning the file type in d_type.  So we leave users to decide
	   * whether these files must be counted or not  */
	  dbg ("(%d) %s/%s (unknown file)\n", deep, folder, dp->d_name);
	  if (flags & READDIR_IGNORE_UNKNOWN)
	    continue;
	  break;
	}

	filecount++;
	dbg ("(%d)  --> #%d\n", deep, filecount);
    }

  dbg ("- return #%d (%s)\n", filecount, folder);
  deep--;
  closedir (dirp);
  return filecount;
}
