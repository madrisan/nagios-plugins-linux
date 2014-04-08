/*  logging.h -- logging facilities
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _LOGGING_H_
#define _LOGGING_H_

void logging (const char *format, ...);

static inline void __attribute__ ((always_inline, format (printf, 1, 2)))
logging_null (const char *format, ...) {}

#ifdef ENABLE_DEBUG
# define dbg(format, ...) fprintf (stdout, "DEBUG: " format, ##__VA_ARGS__)
#else
# define dbg(args...) logging_null(args)
#endif

#endif /* _LOGGING_H */
