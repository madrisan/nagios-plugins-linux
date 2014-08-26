/* messages.h -- a library for uniform output and error messages

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

#ifndef _MESSAGES_H
#define _MESSAGES_H	1

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* Print a message with 'fprintf (stderr, FORMAT, ...)';
     if ERRNUM is nonzero, follow it with ": " and strerror (ERRNUM).
     If STATUS is nonzero, terminate the program with 'exit (STATUS)'.  */
  void plugin_error (nagstatus status, int errnum,
		     const char *message, ...)
       _attribute_format_printf_(3, 4);

  /* This variable is incremented each time 'error' is called.  */
  extern unsigned int error_message_count;

  const char *state_text (nagstatus status);

#ifdef __cplusplus
}
#endif

#endif /* _MESSAGES_H */
