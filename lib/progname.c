#include <stdio.h>
#include <string.h>

#include "common.h"
#include "messages.h"

/* String containing name the program is called with.
   To be initialized by main().  */
const char *program_name = NULL;

/* This string can be used in the final output message */
const char *program_name_short = NULL;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
void
set_program_name (const char *argv0)
{
  const char *slash, *underscore;

  if (argv0 == NULL)
    {
      /* It's a bug in the invoking program.  Help diagnosing it.  */
      plugin_error (STATE_UNKNOWN, 0,
                    "A NULL argv[0] was passed through an exec system call");
    }

  slash = strrchr (argv0, '/');
  program_name = (slash != NULL ? slash + 1 : argv0);

  underscore = strchr (program_name, '_');
  program_name_short = (underscore != NULL ? underscore + 1 : program_name);
}
