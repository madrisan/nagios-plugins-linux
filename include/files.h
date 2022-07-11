// SPDX-License-Identifier: GPL-3.0-or-later
/* files.h -- Linux Pressure Stall Information (PSI) parser

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

#ifndef _FILES_H_
#define _FILES_H_

#ifdef __cplusplus
extern "C"
{
#endif
  enum
  {
    READDIR_DEFAULT            = 0,
    READDIR_DIRECTORIES_ONLY   = (1 << 0),
    READDIR_IGNORE_SYMLINKS    = (1 << 1),
    READDIR_IGNORE_UNKNOWN     = (1 << 2),
    READDIR_RECURSIVE          = (1 << 3),
    READDIR_REGULAR_FILES_ONLY = (1 << 4)
  };

  int files_filecount (const char *folder, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif				/* files.h */
