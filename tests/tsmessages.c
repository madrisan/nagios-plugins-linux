/*
 * License: GPLv3+
 * Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * Unit test for lib/messages.c
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

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "messages.h"
#include "testutils.h"

#define TEST_MSG_STRING(type) \
	TEST_ASSERT_EQUAL_STRING(state_text (STATE_##type), #type)

static int
mymain (void)
{
  int ret = 0;

  TEST_ASSERT_EQUAL_NUMERIC(STATE_OK, 0);
  TEST_ASSERT_EQUAL_NUMERIC(STATE_WARNING, 1);
  TEST_ASSERT_EQUAL_NUMERIC(STATE_CRITICAL, 2);
  TEST_ASSERT_EQUAL_NUMERIC(STATE_UNKNOWN, 3);
  TEST_ASSERT_EQUAL_NUMERIC(STATE_DEPENDENT, 4);

  TEST_MSG_STRING(OK);
  TEST_MSG_STRING(WARNING);
  TEST_MSG_STRING(CRITICAL);
  TEST_MSG_STRING(UNKNOWN);
  TEST_MSG_STRING(DEPENDENT);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

TEST_MAIN(mymain);
