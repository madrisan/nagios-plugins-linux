/*
 * License: GPLv3+
 * Copyright (c) 2016,2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Basic test utilities for the Nagios Plugin Linux.
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
 *
 * This software is based on the source code of the tool "vmstat".
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE		/* activate extra prototypes for glibc */
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "progname.h"
#include "system.h"
#include "testutils.h"

static size_t test_counter;

/* Check that a file is regular and has executable bits.
 * If false is returned, errno is valid. */
static bool
test_file_is_executable (const char *file)
{
  struct stat sb;

  if (stat (file, &sb) < 0)
    return false;

  if (S_ISREG (sb.st_mode) && (sb.st_mode & 0111) != 0)
    return true;

  errno = S_ISDIR (sb.st_mode) ? EISDIR : EACCES;
  return false;
}

static int
test_use_terminal_colors (void)
{
  return isatty (STDIN_FILENO);
}

int
test_main (int argc, char **argv, int (*func) (void), ...)
{
  const char *lib;
  va_list ap;
  int ret;

  va_start (ap, func);
  while ((lib = va_arg(ap, const char *)))
    TEST_PRELOAD (lib);
  va_end(ap);

  program_name = argv[0];
  ret = (func) ();

  return ret;
}

int
test_run (const char *title, int (*body) (const void *data), const void *data)
{
  int ret = body (data);
  test_counter++;

  fprintf (stderr, "%2zu) %-65s ... ", test_counter, title);

  if (test_use_terminal_colors ())
    fprintf (stderr, "%s\n",
	     (ret == 0) ? "\e[32mOK\e[0m" :			/* green */
	     (ret == EXIT_AM_SKIP) ? "\e[34m\e[1mSKIP\e[0m" :	/* bold blue */
	     (ret == EXIT_AM_HARDFAIL) ?			/* bold red */
	      "\e[31m\e[1mHARDFAIL\e[0m" : "\e[31m\e[1mFAILED\e[0m");
  else
    fprintf (stderr, "%s\n",
	     (ret == 0) ? "OK" :
	     (ret == EXIT_AM_SKIP) ? "SKIP" :
	     (ret == EXIT_AM_HARDFAIL) ? "HARDFAIL" : "FAILED");

  return ret;
}
