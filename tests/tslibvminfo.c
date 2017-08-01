/*
 * License: GPLv3+
 * Copyright (c) 2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/vminfo.c
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

#include "common.h"
#include "testutils.h"

# define NPL_TESTING
#  include "../lib/vminfo.c"
# undef NPL_TESTING

proc_vmem_t *vmem = NULL;

static int
test_memory_init ()
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCVMSTAT";

  ret = proc_vmem_new (&vmem);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  ret = setenv (env_variable, NPL_TEST_PATH_PROCVMSTAT, 1);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  proc_vmem_read (vmem);
  unsetenv (env_variable);

  return 0;
}

static void
test_memory_release ()
{
  proc_vmem_unref (vmem);
}

typedef struct test_data
{
  unsigned long value;
  unsigned long expect_value;
} test_data;

static int
test_procvminfo (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC (data->value, data->expect_value);
  return ret;
}

#define test_memory_label(arg, value) \
static int test_memory_ ## arg (const void *tdata)           \
{                                                            \
  int err, ret = 0;                                          \
  unsigned long kb_value;                                    \
  const char *env_variable = "NPL_TEST_PATH_PROCVMSTAT";     \
                                                             \
  /* read the data from (a static copy of) /proc/vmstat */   \
  err = setenv (env_variable, NPL_TEST_PATH_PROCVMSTAT, 1);  \
  if (err < 0)                                               \
    return EXIT_AM_HARDFAIL;                                 \
                                                             \
  kb_value = proc_vmem_get_ ## arg (vmem);                   \
  TEST_ASSERT_EQUAL_NUMERIC (kb_value, value);               \
                                                             \
  return ret;                                                \
}

test_memory_label (pgpgin, 2581510UL);
test_memory_label (pgpgout, 34298109UL);
test_memory_label (pswpin, 10402UL);
test_memory_label (pswpout, 12250UL);
test_memory_label (pgfault, 91270548UL);
test_memory_label (pgmajfault, 9363UL);
test_memory_label (pgfree, 91315814UL);
/*test_memory_label (pgsteal, 0UL);*/
/*test_memory_label (pgscand, 0UL);*/
/*test_memory_label (pgscank, 0UL);*/

static int
mymain (void)
{
  int err, ret = 0;

  if ((err = test_memory_init (&vmem)) != 0)
    return err;

  proc_vmem_data_t *procdata = vmem->data;

#define DO_TEST_PROC(MEMTYPE, VALUE, EXPECT_VALUE)                       \
  do                                                                     \
    {                                                                    \
      test_data data = {                                                 \
        .value = VALUE,                                                  \
        .expect_value = EXPECT_VALUE,                                    \
      };                                                                 \
      if (test_run("check " PROC_STAT " parser for " MEMTYPE " memory",  \
		   test_procvminfo, (&data)) < 0)                        \
        ret = -1;                                                        \
    }                                                                    \
  while (0)

  /* test the proc parser */

  DO_TEST_PROC ("allocstall", procdata->vm_allocstall, 0L);
  DO_TEST_PROC ("kswapd_inodesteal", procdata->vm_kswapd_inodesteal, 0L);
  /* kswapd_steal */
  DO_TEST_PROC ("nr_dirty", procdata->vm_nr_dirty, 20L);
  DO_TEST_PROC ("nr_mapped", procdata->vm_nr_mapped, 314943L);
  DO_TEST_PROC ("nr_page_table_pages",
		procdata->vm_nr_page_table_pages, 10601L);
  /* nr_pagecache */
  /* nr_reverse_maps */
  /* nr_slab */
  DO_TEST_PROC ("nr_unstable", procdata->vm_nr_unstable, 0L);
  DO_TEST_PROC ("nr_writeback", procdata->vm_nr_writeback, 0L);
  DO_TEST_PROC ("pageoutrun", procdata->vm_pageoutrun, 1L);
  DO_TEST_PROC ("pgactivate", procdata->vm_pgactivate, 764549L);
  /* pgalloc */
  DO_TEST_PROC ("pgalloc_dma", procdata->vm_pgalloc_dma, 6L);
  /* pgalloc_high */
  DO_TEST_PROC ("pgalloc_normal", procdata->vm_pgalloc_normal, 70896125L);
  DO_TEST_PROC ("pgdeactivate", procdata->vm_pgdeactivate, 0L);
  DO_TEST_PROC ("pgfault", procdata->vm_pgfault, 91270548L);
  DO_TEST_PROC ("pgfree", procdata->vm_pgfree, 91315814L);
  DO_TEST_PROC ("pginodesteal", procdata->vm_pginodesteal, 0L);
  DO_TEST_PROC ("pgmajfault", procdata->vm_pgmajfault, 9363L);
  DO_TEST_PROC ("pgpgin", procdata->vm_pgpgin, 2581510L);
  DO_TEST_PROC ("pgpgout", procdata->vm_pgpgout, 34298109L);
  DO_TEST_PROC ("pgrefill", procdata->vm_pgrefill, 0L);
  DO_TEST_PROC ("pgrefill_dma", procdata->vm_pgrefill_dma, 0L);
  /* pgrefill_high */
  DO_TEST_PROC ("pgrefill_normal", procdata->vm_pgrefill_normal, 0L);
  DO_TEST_PROC ("pgrotated", procdata->vm_pgrotated, 407L);
  DO_TEST_PROC ("pgscan", procdata->vm_pgscan, 0L);
  DO_TEST_PROC ("pgscan_direct_dma", procdata->vm_pgscan_direct_dma, 0L);
  /* pgscan_direct_high */
  DO_TEST_PROC ("pgscan_direct_normal", procdata->vm_pgscan_direct_normal, 0L);
  DO_TEST_PROC ("pgscan_kswapd_dma", procdata->vm_pgscan_kswapd_dma, 0L);
  /* pgscan_kswapd_high */
  DO_TEST_PROC ("pgscan_kswapd_normal", procdata->vm_pgscan_kswapd_normal, 0L);
  DO_TEST_PROC ("pgsteal", procdata->vm_pgsteal, 0L);
  /* pgsteal_dma */
  /* pgsteal_high */
  /* pgsteal_normal */
  DO_TEST_PROC ("pswpin", procdata->vm_pswpin, 10402L);
  DO_TEST_PROC ("pswpout", procdata->vm_pswpout, 12250L);
  DO_TEST_PROC ("slabs_scanned", procdata->vm_slabs_scanned, 0L);

  /* test the public interface of the library */

#define DO_TEST(MSG, FUNC, DATA) \
  do { if (test_run (MSG, FUNC, DATA) < 0) ret = -1; } while (0)

  DO_TEST ("check pgpgin virtual memory stat", test_memory_pgpgin, NULL);
  DO_TEST ("check pgpgout virtual memory stat", test_memory_pgpgout, NULL);

  /* used by check_swap, check_paging */
  DO_TEST ("check pswpin virtual memory stat", test_memory_pswpin, NULL);
  DO_TEST ("check pswpout virtual memory stat", test_memory_pswpout, NULL);

  /* used by check_paging */
  DO_TEST ("check pgfault virtual memory stat", test_memory_pgfault, NULL);
  DO_TEST ("check pgmajfault virtual memory stat", test_memory_pgmajfault, NULL);
  DO_TEST ("check pgfree virtual memory stat", test_memory_pgfree, NULL);
  /*DO_TEST ("check pgsteal virtual memory stat", test_memory_pgsteal, NULL);*/
  /*DO_TEST ("check pgscand virtual memory stat", test_memory_pgscand, NULL);*/
  /*DO_TEST ("check pgscank virtual memory stat", test_memory_pgscank, NULL);*/

  test_memory_release (vmem);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
