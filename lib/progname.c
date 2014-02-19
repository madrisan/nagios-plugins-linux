#include "config.h"

#include <stdio.h>

#include "error.h"
#include "nputils.h"

/* String containing name the program is called with.
   To be initialized by main().  */
const char *program_name = NULL;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
void
set_program_name (const char *argv0)
{

  if (argv0 == NULL)
    {
      /* It's a bug in the invoking program.  Help diagnosing it.  */
      plugin_error (STATE_UNKNOWN, 0,
                    "A NULL argv[0] was passed through an exec system call");
    }

  program_name = argv0;
}
