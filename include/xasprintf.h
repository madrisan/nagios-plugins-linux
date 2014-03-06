/* asprintf with out-of-memory checking.

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

#ifndef _XASPRINTF_H
#define _XASPRINTF_H        1

#ifdef __cplusplus
extern "C" {
#endif

/* Write formatted output to a string dynamically allocated with malloc(),
   and return it.  On error exit with the STATE_UNKNOWN status  */
extern char *xasprintf (const char *format, ...)
       _attribute_format_printf_(1, 2);

#ifdef __cplusplus
}
#endif

#endif /* _XASPRINTF_H */
