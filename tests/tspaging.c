/*
 * License: GPLv3+
 * Copyright (c) 2016,2017 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for check_paging.c
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

/* silence the compiler's warning 'function defined but not used' */
static _Noreturn void print_version (void) __attribute__((unused));
static _Noreturn void usage (FILE * out) __attribute__((unused));

#define NPL_TESTING
# include "../plugins/check_paging.c"
#undef NPL_TESTING

static int
test_paging_monotonic (const void *tdata)
{
  bool show_swapping = false;
  bool swapping_only = false;
  paging_data_t paging;
  int ret = 0;

  get_paging_status (show_swapping, swapping_only, &paging);

#define CHECK_IF_POSITIVE(struct_member) \
 { if (struct_member < 0) ret = -1; } while (0)
  CHECK_IF_POSITIVE (paging.dpgpgin);
  CHECK_IF_POSITIVE (paging.dpgpgout);
  CHECK_IF_POSITIVE (paging.dpgfault);
  CHECK_IF_POSITIVE (paging.dpgfree);
  CHECK_IF_POSITIVE (paging.dpgmajfault);
  CHECK_IF_POSITIVE (paging.dpgscand);
  CHECK_IF_POSITIVE (paging.dpgscank);
  CHECK_IF_POSITIVE (paging.dpgsteal);
  CHECK_IF_POSITIVE (paging.dpswpin);
  CHECK_IF_POSITIVE (paging.dpswpout);

  return ret;
}

static int
mymain (void)
{
  int ret = 0;

#define DO_TEST(MSG, FUNC, DATA) \
  do { if (test_run (MSG, FUNC, DATA) < 0) ret = -1; } while (0)

  DO_TEST ("check if get_paging_status() is monotonic",
           test_paging_monotonic, NULL);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN (mymain)
