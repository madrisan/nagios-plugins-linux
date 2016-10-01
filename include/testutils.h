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

#define EXIT_AM_SKIP 77		/* tell Automake we're skipping a test */
#define EXIT_AM_HARDFAIL 99	/* tell Automake that the framework is broken */

#define TEST_ASSERT_EQUAL_NUMERIC(A, B)  \
  do {                                   \
    if (A != B) ret = -1;                \
  } while (0)

#define TEST_ASSERT_EQUAL_STRING(A, B)   \
  do {                                   \
    if (strcmp(A, B) != 0) ret = -1;     \
  } while (0)

#ifdef __cplusplus
extern "C"
{
#endif

  int test_main (int argc, char **argv, int (*func) (void), ...);

  #define TEST_MAIN(func)                           \
    int main(int argc, char **argv) {               \
      return test_main(argc, argv, func, NULL);     \
    }

  int test_run (const char *title,
		int (*body) (const void *data), const void *data);

#ifdef __cplusplus
}
#endif

#endif				/* _TESTUTILS_H */
