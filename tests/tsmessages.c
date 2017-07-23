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

struct test_data
{
  char *state;
  nagstatus value;
  nagstatus shouldbe;
};

static int
test_nagios_state_string (const void *tdata)
{
  const struct test_data *data = tdata;
  int ret = 0;

  TEST_ASSERT_EQUAL_STRING (state_text (data->value), data->state);
  TEST_ASSERT_EQUAL_NUMERIC (data->value, data->shouldbe);
  return ret;
}

#define STR(S) #S
#define TEST_NAGIOS_STATUS(FUNC, TYPE, SHOULD_BE)               \
  do                                                            \
    {                                                           \
      data.state = STR(TYPE);                                   \
      data.value = STATE_##TYPE;                                \
      data.shouldbe = SHOULD_BE;                                \
      TEST_DATA ("check nagios state " STR(TYPE), FUNC, &data); \
    }                                                           \
  while (0)

static int
mymain (void)
{
  int ret = 0;
  struct test_data data;

  TEST_NAGIOS_STATUS (test_nagios_state_string, OK, 0);
  TEST_NAGIOS_STATUS (test_nagios_state_string, WARNING, 1);
  TEST_NAGIOS_STATUS (test_nagios_state_string, CRITICAL, 2);
  TEST_NAGIOS_STATUS (test_nagios_state_string, UNKNOWN, 3);
  TEST_NAGIOS_STATUS (test_nagios_state_string, DEPENDENT, 4);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#undef TEST_NAGIOS_STATUS
#undef STR

TEST_MAIN (mymain);
