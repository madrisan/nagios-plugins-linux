// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/files.c
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
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "testutils.h"
#include "xasprintf.h"

# define NPL_TESTING
#  include "../lib/files.c"
# undef NPL_TESTING

typedef struct test_data
{
  char *basedir;
  unsigned int flags;
  int64_t age;
  int64_t size;
  char *pattern;
  int64_t expect_value;
} test_data;

typedef struct environment
{
  char *name;
  char *target;
  unsigned int flags;
} test_environment;

static test_environment const env[] = {
  {(char *) "1", NULL, S_IFREG },
  {(char *) "2", NULL, S_IFREG },
  {(char *) "3", NULL, S_IFREG },
  {(char *) "4", NULL, S_IFREG },
  {(char *) "5", NULL, S_IFREG },
  {(char *) "6", NULL, S_IFREG },
  {(char *) "link1", "1", S_IFLNK},
  {(char *) ".hiddenfile", NULL, S_IFREG },
  {(char *) "a", NULL, S_IFDIR},
  {(char *) "a/.hiddendir", NULL, S_IFDIR},
  {(char *) "a/1", NULL, S_IFREG },
  {(char *) "a/2", NULL, S_IFREG },
  {(char *) "a/3", NULL, S_IFREG },
  {(char *) "a/b", NULL, S_IFDIR},
  {(char *) "a/link2", "2", S_IFLNK},
  {(char *) "a/b/1", NULL, S_IFREG},
  {(char *) "a/b/2", NULL, S_IFREG},
  {(char *) "a/b/3", NULL, S_IFREG},
  {(char *) "a/b/4", NULL, S_IFREG},
  {(char *) "a/b/link3", "../3", S_IFLNK},
  {NULL, 0}
};

static int
test_files_filecount (const void *tdata)
{
  const struct test_data *data = tdata;
  struct files_types *filecount = NULL;
  int ret = 0;

  ret = files_filecount (data->basedir, data->flags, data->age, data->size,
			 data->pattern, &filecount);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  TEST_ASSERT_EQUAL_NUMERIC (filecount->total, data->expect_value);

  free (filecount);
  return ret;
}

static int
test_create_tree (char **basedir)
{
  char template[] = "/tmp/tslibfiles_filecount.XXXXXX";
  test_environment e = env[0];
  int i = 0;

  *basedir = mkdtemp (template);
  if (NULL == basedir)
    {
      perror ("mkdtemp failed in test_create_tree ()");
      return EXIT_AM_HARDFAIL;
    }
  printf ("\tenvironment: %s\n", *basedir);

  while (e.name)
    {
      char *path = xasprintf ("%s/%s", *basedir, e.name);
      switch (e.flags)
	{
	default:
	  perror ("reached default in test_create_tree ()");
	  return EXIT_AM_HARDFAIL;
	case S_IFDIR:
	  printf ("\t- directory: %s\n", e.name);
	  if (mkdir (path, S_IRWXU) < 0)
	    {
	      perror ("- mkdir failed in test_create_tree ()");
	      return EXIT_AM_HARDFAIL;
	    }
	  break;
	case S_IFLNK:
	  printf ("\t- symlink  : %s\n", e.name);
	  if (symlink (e.target, path) == -1)
	    {
	      perror ("symlink failed in test_create_tree ()");
	      return EXIT_AM_HARDFAIL;
	    }
	  break;
	case S_IFREG:
	  printf ("\t- reg file : %s\n", e.name);
          if ((open (path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
	    {
	      perror ("open failed in test_create_tree ()");
	      return EXIT_AM_HARDFAIL;
	    }
	  break;
	}

      free (path);
      e = env[++i];
    }

  return 0;
}

static int
mymain (void)
{
  int ret = 0;
  char *basedir;

# define DO_TEST(TEST, BASEDIR, FLAGS, AGE, SIZE, PATTERN, EXPECT_VALUE) \
  do                                                                     \
    {                                                                    \
      test_data data = {                                                 \
        .basedir = BASEDIR,                                              \
        .flags = FLAGS,                                                  \
        .age = AGE,                                                      \
        .size = SIZE,                                                    \
        .pattern = PATTERN,                                              \
        .expect_value = EXPECT_VALUE,                                    \
      };                                                                 \
      if (test_run("check function files_filecount (" TEST ")",          \
                   test_files_filecount, (&data)) < 0)                   \
        ret = -1;                                                        \
    }                                                                    \
  while (0)

  /* test the function files_filecount() */

  ret = test_create_tree (&basedir);
  if (ret != 0)
    return ret;

  DO_TEST ("default",
	   basedir,
	   FILES_DEFAULT,
	   0, 0, NULL, 8);
  DO_TEST ("default + hidden",
	   basedir,
	   FILES_DEFAULT | FILES_INCLUDE_HIDDEN,
	   0, 0, NULL, 9);
  DO_TEST ("default - symlinks",
	   basedir,
	   FILES_DEFAULT | FILES_IGNORE_SYMLINKS,
	   0, 0, NULL, 7);
  DO_TEST ("regular only",
	   basedir,
	   FILES_REGULAR_ONLY,
	   0, 0, NULL, 6);
  DO_TEST ("regular + hidden",
	   basedir,
	   FILES_REGULAR_ONLY | FILES_INCLUDE_HIDDEN,
	   0, 0, NULL, 7);
  DO_TEST ("regular + symlinks",
	   basedir,
	   FILES_REGULAR_ONLY | FILES_IGNORE_SYMLINKS,
	   0, 0, NULL, 6);
  DO_TEST ("recursive",
	   basedir,
	   FILES_RECURSIVE,
	   0, 0, NULL, 18);
  DO_TEST ("recursive + regular",
	   basedir,
	   FILES_RECURSIVE | FILES_REGULAR_ONLY,
	   0, 0, NULL, 13);
  DO_TEST ("recursive + hidden",
	   basedir,
	   FILES_RECURSIVE | FILES_INCLUDE_HIDDEN,
	   0, 0, NULL, 20);
  DO_TEST ("recursive - symlinks",
	   basedir,
	   FILES_RECURSIVE | FILES_IGNORE_SYMLINKS,
	   0, 0, NULL, 15);
  DO_TEST ("recursive + hidden - symlinks",
	   basedir,
	   FILES_RECURSIVE | FILES_INCLUDE_HIDDEN | FILES_IGNORE_SYMLINKS,
	   0, 0, NULL, 17);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
