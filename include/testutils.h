/* testutils.h -- a simple framework for unit testing

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _TESTUTILS_H
#define _TESTUTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "getenv.h"

#define EXIT_AM_SKIP 77		/* tell Automake we're skipping a test */
#define EXIT_AM_HARDFAIL 99	/* tell Automake that the framework is broken */

#define TEST_KERNEL_VERSION_MAJOR 2
#define TEST_KERNEL_VERSION_MINOR 6
#define TEST_KERNEL_VERSION_PATCH 27
#define TEST_KERNEL_VERSION "2.6.27"

/* environment variables to allow testing via static proc files;
   note that 'abs_srcdir' seems the only way to make 'make distcheck' happy
   (we cannot use 'abs_builddir' because the *.data files are not copied to
   <pckrootdir>/nagios-plugins-linux-<version>/_build/sub/tests/) */
#define NPL_TEST_PATH_PROCSTAT abs_srcdir "/ts_procstat.data"
#define NPL_TEST_PATH_PROCMEMINFO abs_srcdir "/ts_procmeminfo.data"
#define NPL_TEST_PATH_PROCVMSTAT abs_srcdir "/ts_procvmstat.data"

#define TEST_ASSERT_EQUAL_NUMERIC(A, B) \
  do { if ((A) != (B)) ret = -1; } while (0)

#define TEST_ASSERT_EQUAL_STRING(A, B) \
  do { if (strcmp (A, B) != 0) ret = -1; } while (0)

#ifdef __cplusplus
extern "C"
{
#endif

  int test_main (int argc, char **argv, int (*func) (void), ...);

# define TEST_MAIN(func)                                            \
    int main (int argc, char **argv)                                \
    {                                                               \
      return test_main (argc, argv, func, NULL);                    \
    }

# define TEST_PRELOAD(lib)                                          \
    do                                                              \
      {                                                             \
	const char *preload = secure_getenv ("LD_PRELOAD");         \
	if (preload == NULL || strstr (preload, lib) == NULL)       \
	  {                                                         \
	    char *newenv;                                           \
	    if (!test_file_is_executable (lib))                     \
	      {                                                     \
		perror (lib);                                       \
		return EXIT_FAILURE;                                \
	      }                                                     \
	    if (!preload)                                           \
	      {                                                     \
		newenv = (char *) lib;                              \
	      }                                                     \
	    else if (asprintf (&newenv, "%s:%s", lib, preload) < 0) \
	      {                                                     \
		perror ("asprintf");                                \
		return EXIT_FAILURE;                                \
	      }                                                     \
	    if (setenv ("LD_PRELOAD", newenv, 1) < 0)               \
	      {                                                     \
		perror ("setenv");                                  \
		return EXIT_FAILURE;                                \
	      }                                                     \
	    char *argv0 = realpath (argv[0], NULL);                 \
	    if (!argv0)                                             \
	      {                                                     \
		perror ("realpath");                                \
		return EXIT_FAILURE;                                \
	      }                                                     \
	    execv (argv0, argv);                                    \
	    free (argv0);                                           \
	  }                                                         \
      }                                                             \
    while (0)

# define TEST_MAIN_PRELOAD(func, ...)                               \
    int main (int argc, char **argv)                                \
    {                                                               \
      return test_main (argc, argv, func, __VA_ARGS__, NULL);       \
    }

  /* Runs test.
     returns: -1 = error, 0 = success  */
  int test_run (const char *title,
		int (*body) (const void *data), const void *data);

#ifdef __cplusplus
}
#endif

#endif				/* _TESTUTILS_H */
