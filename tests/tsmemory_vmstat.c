/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_memory.c and check_swap.c (/proc/vmstat)
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
#include "vminfo.h"
#include "testutils.h"

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__((unused));
static _Noreturn void usage (FILE * out) __attribute__((unused));

#define NPL_TESTING
# include "../plugins/check_memory.c"
#undef NPL_TESTING

static struct proc_vmem *vmem = NULL;

static int
test_memory_init ()
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCVMSTAT";

  if (vmem == NULL)
    {
      ret = proc_vmem_new (&vmem);
      if (ret < 0)
	return EXIT_AM_HARDFAIL;

      ret = setenv (env_variable, NPL_TEST_PATH_PROCVMSTAT, 1);
      if (ret < 0)
	return EXIT_AM_HARDFAIL;

      proc_vmem_read (vmem);
      unsetenv (env_variable);
    }

  return 0;
}

static void
test_memory_release ()
{
  proc_vmem_unref (vmem);
}

#define test_memory_label(arg, value) \
static int test_memory_ ## arg (const void *tdata)           \
{                                                            \
  int err, ret = 0;                                          \
  unsigned long kb_value;                                    \
  const char *env_variable = "NPL_TEST_PATH_PROCVMSTAT";     \
                                                             \
  err = setenv (env_variable, NPL_TEST_PATH_PROCVMSTAT, 1);  \
  if (err < 0)                                               \
    return EXIT_AM_HARDFAIL;                                 \
                                                             \
  kb_value = proc_vmem_get_ ## arg (vmem);                   \
  TEST_ASSERT_EQUAL_NUMERIC(kb_value, value);                \
                                                             \
  return ret;                                                \
}

test_memory_label (pgpgin, 2581510UL);
test_memory_label (pgpgout, 34298109UL);
test_memory_label (pswpin, 10402UL);
test_memory_label (pswpout, 12250UL);

static int
mymain (void)
{
  int err, ret = 0;

  if ((err = test_memory_init ()) != 0)
    return err;

  /* data from (a static copy of) /proc/vmstat */

  TEST_DATA ("check pgpgin virtual memory stat", test_memory_pgpgin, NULL);
  TEST_DATA ("check pgpgout virtual memory stat", test_memory_pgpgout, NULL);
  TEST_DATA ("check pswpin virtual memory stat", test_memory_pswpin, NULL);
  TEST_DATA ("check pswpin virtual memory stat", test_memory_pswpout, NULL);

  test_memory_release ();

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
