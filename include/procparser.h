// SPDX-License-Identifier: GPL-3.0-or-later
/* procparser.h -- a parser for /proc files, /proc/meminfo, and /proc/vmstat

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _PROCPARSER_H_
#define _PROCPARSER_H_

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct proc_table_struct
  {
    const char *name;		/* proc type name */
    unsigned long *slot;	/* slot in return struct */
  } proc_table_struct;

  void procparser (const char *filename, const proc_table_struct * proc_table,
		   int proc_table_count, char separator);

  /* Lookup a pattern and get the value from line
   * Format is:
   *	 "<pattern>   : <key>"
   */
  int linelookup (char *line, char *pattern, char **value);

#ifdef __cplusplus
}
#endif

#endif				/* _PROCPARSER_H_ */
