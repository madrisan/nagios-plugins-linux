/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_memory.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "memory.h"
#include "testutils.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__((unused));
static _Noreturn void usage (FILE * out) __attribute__((unused));

#define NPL_TESTING
# include "../plugins/check_memory.c"
#undef NPL_TESTING

static struct proc_sysmem *sysmem = NULL;

static int
test_memory_init ()
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCMEMINFO";

  if (sysmem == NULL)
    {
      ret = proc_sysmem_new (&sysmem);
      if (ret < 0)
	return EXIT_AM_HARDFAIL;
  
      ret = setenv (env_variable, NPL_TEST_PATH_PROCMEMINFO, 1);
      if (ret < 0)
	return EXIT_AM_HARDFAIL;

      proc_sysmem_read (sysmem);
      unsetenv (env_variable);
    }

  return 0;
}

static void
test_memory_release ()
{
  proc_sysmem_unref (sysmem);
}

#define TEST_DATA(MSG, FUNC)                                 \
  do {                                                       \
    if (test_run (MSG, FUNC, NULL) < 0)                      \
      ret = -1;                                              \
  } while (0)

#define test_memory_label(arg, value) \
static int test_memory_ ## arg (const void *tdata)           \
{                                                            \
  int err, ret = 0;                                          \
  unsigned long kb_meminfo;                                  \
  const char *env_variable = "NPL_TEST_PATH_PROCMEMINFO";    \
                                                             \
  err = setenv (env_variable, NPL_TEST_PATH_PROCMEMINFO, 1); \
  if (err < 0)                                               \
    return EXIT_AM_HARDFAIL;                                 \
                                                             \
  kb_meminfo = proc_sysmem_get_ ## arg (sysmem);             \
  TEST_ASSERT_EQUAL_NUMERIC(kb_meminfo, value);              \
                                                             \
  return ret;                                                \
}

test_memory_label (active, 3090692UL);
test_memory_label (anon_pages, 1008780UL);
test_memory_label (committed_as, 3678828UL);
test_memory_label (dirty, 92UL);
test_memory_label (inactive, 1044404UL);
test_memory_label (main_available, 14564424UL);
test_memory_label (main_buffers, 384876UL);
test_memory_label (main_free, 11918208UL);
test_memory_label (main_shared, 387476UL);
test_memory_label (main_total, 16384256UL);

static int
mymain (void)
{
  int err, ret = 0;

  if ((err = test_memory_init ()) != 0)
    return err; 

  TEST_DATA("check active memory", test_memory_active);
  TEST_DATA("check anon_pages memory", test_memory_anon_pages);
  TEST_DATA("check committed_as memory", test_memory_committed_as);
  TEST_DATA("check dirty memory", test_memory_dirty);
  TEST_DATA("check inactive memory", test_memory_inactive);
  TEST_DATA("check main_available memory", test_memory_main_available);
  TEST_DATA("check main_buffers memory", test_memory_main_buffers);
  TEST_DATA("check main_free memory", test_memory_main_free);
  TEST_DATA("check main_shared memory", test_memory_main_shared);
  TEST_DATA("check main_total memory", test_memory_main_total);

  test_memory_release ();

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
