/*
 * License: GPLv3+
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This library was based on and inspired by the gnulib code.
 */

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "messages.h"

/* This variable is incremented each time 'error' is called.  */
unsigned int error_message_count;

/* The calling program should define program_name and set it to the
   name of the executing program.  */
extern char *program_name;

/* Return non-zero if FD is open.  */
static inline int
is_open (int fd)
{
  return 0 <= fcntl (fd, F_GETFL);
}

static inline void
flush_stdout (void)
{
  int stdout_fd;

  /* POSIX states that fileno (stdout) after fclose is unspecified.  But in
     practice it is not a problem, because stdout is statically allocated and
     the fd of a FILE stream is stored as a field in its allocated memory.  */
  stdout_fd = fileno (stdout);

  /* POSIX states that fflush (stdout) after fclose is unspecified; it
     is safe in glibc, but not on all other platforms.  fflush (NULL)
     is always defined, but too draconian.  */
  if (0 <= stdout_fd && is_open (stdout_fd))
    fflush (stdout);
}

static void
print_errno_message (int errnum)
{
  char const *s;

  s = strerror (errnum);
  fprintf (stdout, " (%s)", s);
}

/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args.
   If ERRNUM is nonzero, print its corresponding system error message.
   Exit with status STATUS if it is nonzero.  */
void
plugin_error (nagstatus status, int errnum, const char *message, ...)
{
  va_list args;

  flush_stdout ();
  
  fprintf (stdout, "%s: ", program_name);

  va_start (args, message);
  vfprintf (stdout, message, args);
  va_end (args);

  ++error_message_count;
  if (errnum)
    print_errno_message (errnum);

  fputs ("\n", stdout);
  fflush (stdout);

  if (status)
    exit (status);
}

const char *
state_text (nagstatus result)
{
  switch (result)
    {
    case STATE_OK:
      return "OK";
    case STATE_WARNING:
      return "WARNING";
    case STATE_CRITICAL:
      return "CRITICAL";
    case STATE_DEPENDENT:
      return "DEPENDENT";
    default:
      return "UNKNOWN";
    }
}
