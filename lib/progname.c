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
