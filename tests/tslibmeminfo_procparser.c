// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/meminfo.c
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

#include "testutils.h"

#if defined (PROC_MEMINFO) && !defined (HAVE_LIBPROCPS)

# define NPL_TESTING
#  include "../lib/meminfo.c"
# undef NPL_TESTING

proc_sysmem_t *sysmem = NULL;

static int
test_memory_init ()
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCMEMINFO";

  ret = proc_sysmem_new (&sysmem);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  ret = setenv (env_variable, NPL_TEST_PATH_PROCMEMINFO, 1);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  proc_sysmem_read (sysmem);
  unsetenv (env_variable);

  return 0;
}

static void
test_memory_release ()
{
  proc_sysmem_unref (sysmem);
}

typedef struct test_data
{
  unsigned long value;
  unsigned long expect_value;
} test_data;

static int
test_procmeminfo (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC (data->value, data->expect_value);
  return ret;
}

static int
mymain (void)
{
  int err, ret = 0;

  if ((err = test_memory_init ()) != 0)
    return err;

  proc_sysmem_data_t *procdata = sysmem->data;

# define DO_TEST(MEMTYPE, VALUE, EXPECT_VALUE)                              \
  do                                                                        \
    {                                                                       \
      test_data data = {                                                    \
	.value = VALUE,                                                     \
	.expect_value = EXPECT_VALUE,                                       \
      };                                                                    \
      if (test_run("check " PROC_MEMINFO " parser for " MEMTYPE " memory",  \
		   test_procmeminfo, (&data)) < 0)                          \
	ret = -1;                                                           \
    }                                                                       \
  while (0)

  /* test the proc parser */

  DO_TEST ("Active", procdata->kb_active, 3090692UL);
  DO_TEST ("Active(file)", procdata->kb_active_file, 2021912UL);
  DO_TEST ("AnonPages", procdata->kb_anon_pages, 1008780UL);
  DO_TEST ("Buffers", procdata->kb_main_buffers, 384876UL);
  DO_TEST ("Cached", procdata->kb_page_cache, 2741476UL);
  DO_TEST ("Committed_AS", procdata->kb_committed_as, 3678828UL);
  DO_TEST ("Dirty", procdata->kb_dirty, 92UL);
  DO_TEST ("HighTotal", procdata->kb_high_total, 0UL);
  DO_TEST ("Inact_clean", procdata->kb_inact_clean, 0UL);
  DO_TEST ("Inact_dirty", procdata->kb_inact_dirty, 0UL);
  DO_TEST ("Inact_laundry", procdata->kb_inact_laundry, 0UL);
  DO_TEST ("Inactive", procdata->kb_inactive, 1044404UL);
  DO_TEST ("Inactive(file)", procdata->kb_inactive_file, 716976UL);
  DO_TEST ("LowFree", procdata->kb_low_free, 0UL);
  DO_TEST ("LowTotal", procdata->kb_low_total, MEMINFO_UNSET);
  DO_TEST ("MemAvailable", procdata->kb_main_available, 14564424UL);
  DO_TEST ("MemFree", procdata->kb_main_free, 11918208UL);
  DO_TEST ("MemTotal", procdata->kb_main_total, 16384256UL);
  DO_TEST ("SReclaimable", procdata->kb_slab_reclaimable, 152528UL);
  DO_TEST ("Shmem", procdata->kb_main_shared, 387476UL);
  DO_TEST ("Slab", procdata->kb_slab, 202816UL);
  DO_TEST ("SwapCached", procdata->kb_swap_cached, 1024UL);
  DO_TEST ("SwapFree", procdata->kb_swap_free, 8387580UL);
  DO_TEST ("SwapTotal", procdata->kb_swap_total, 8388604UL);

  test_memory_release ();

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)

#else

int
main (void)
{
  return EXIT_AM_SKIP;
}

#endif
