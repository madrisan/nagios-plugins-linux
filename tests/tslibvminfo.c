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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "testutils.h"

# define NPL_TESTING
#  include "../lib/vminfo.c"
# undef NPL_TESTING

static int
test_memory_init (struct proc_vmem **vmem)
{
  int ret = 0;
  const char *env_variable = "NPL_TEST_PATH_PROCVMSTAT";

  ret = proc_vmem_new (vmem);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  ret = setenv (env_variable, NPL_TEST_PATH_PROCVMSTAT, 1);
  if (ret < 0)
    return EXIT_AM_HARDFAIL;

  proc_vmem_read (*vmem);
  unsetenv (env_variable);

  return 0;
}

static void
test_memory_release (struct proc_vmem *vmem)
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

static int
mymain (void)
{
  proc_vmem_t *vmem = NULL;
  int err, ret = 0;

  if ((err = test_memory_init (&vmem)) != 0)
    return err;

  proc_vmem_data_t *procdata = vmem->data;

# define DO_TEST(MEMTYPE, VALUE, EXPECT_VALUE)                           \
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

  DO_TEST ("allocstall", procdata->vm_allocstall, 0L);
  DO_TEST ("kswapd_inodesteal", procdata->vm_kswapd_inodesteal, 0L);
  /* kswapd_steal */
  DO_TEST ("nr_dirty", procdata->vm_nr_dirty, 20L);
  DO_TEST ("nr_mapped", procdata->vm_nr_mapped, 314943L);
  DO_TEST ("nr_page_table_pages", procdata->vm_nr_page_table_pages, 10601L);
  /* nr_pagecache */
  /* nr_reverse_maps */
  /* nr_slab */
  DO_TEST ("nr_unstable", procdata->vm_nr_unstable, 0L);
  DO_TEST ("nr_writeback", procdata->vm_nr_writeback, 0L);
  DO_TEST ("pageoutrun", procdata->vm_pageoutrun, 1L);
  DO_TEST ("pgactivate", procdata->vm_pgactivate, 764549L);
  /* pgalloc */
  DO_TEST ("pgalloc_dma", procdata->vm_pgalloc_dma, 6L);
  /* pgalloc_high */
  DO_TEST ("pgalloc_normal", procdata->vm_pgalloc_normal, 70896125L);
  DO_TEST ("pgdeactivate", procdata->vm_pgdeactivate, 0L);
  DO_TEST ("pgfault", procdata->vm_pgfault, 91270548L);
  DO_TEST ("pgfree", procdata->vm_pgfree, 91315814L);
  DO_TEST ("pginodesteal", procdata->vm_pginodesteal, 0L);
  DO_TEST ("pgmajfault", procdata->vm_pgmajfault, 9363L);
  DO_TEST ("pgpgin", procdata->vm_pgpgin, 2581510L);
  DO_TEST ("pgpgout", procdata->vm_pgpgout, 34298109L);
  DO_TEST ("pgrefill", procdata->vm_pgrefill, 0L);
  DO_TEST ("pgrefill_dma", procdata->vm_pgrefill_dma, 0L);
  /* pgrefill_high */
  DO_TEST ("pgrefill_normal", procdata->vm_pgrefill_normal, 0L);
  DO_TEST ("pgrotated", procdata->vm_pgrotated, 407L);
  DO_TEST ("pgscan", procdata->vm_pgscan, 0L);
  DO_TEST ("pgscan_direct_dma", procdata->vm_pgscan_direct_dma, 0L);
  /* pgscan_direct_high */
  DO_TEST ("pgscan_direct_normal", procdata->vm_pgscan_direct_normal, 0L);
  DO_TEST ("pgscan_kswapd_dma", procdata->vm_pgscan_kswapd_dma, 0L);
  /* pgscan_kswapd_high */
  DO_TEST ("pgscan_kswapd_normal", procdata->vm_pgscan_kswapd_normal, 0L);
  DO_TEST ("pgsteal", procdata->vm_pgsteal, 0L);
  /* pgsteal_dma */
  /* pgsteal_high */
  /* pgsteal_normal */
  DO_TEST ("pswpin", procdata->vm_pswpin, 10402L);
  DO_TEST ("pswpout", procdata->vm_pswpout, 12250L);
  DO_TEST ("slabs_scanned", procdata->vm_slabs_scanned, 0L);

  test_memory_release (vmem);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
