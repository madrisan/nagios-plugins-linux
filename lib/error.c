#include "config.h"
#include "error.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* This variable is incremented each time 'error' is called.  */
unsigned int error_message_count;

/* The calling program should define program_name and set it to the
   name of the executing program.  */
extern const char *program_name;

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
  fprintf (stderr, ": %s", s);
}

/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args.
   If ERRNUM is nonzero, print its corresponding system error message.
   Exit with status STATUS if it is nonzero.  */
void
error (int status, int errnum, const char *message, ...)
{
  va_list args;

  flush_stdout ();
  
  fprintf (stderr, "%s: ", program_name);

  va_start (args, message);
  vfprintf (stderr, message, args);
  va_end (args);

  ++error_message_count;
  if (errnum)
    print_errno_message (errnum);

  fflush (stderr);
  if (status)
    exit (status);
}
