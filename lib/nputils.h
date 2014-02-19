#pragma once

enum
{
  STATE_OK,
  STATE_WARNING,
  STATE_CRITICAL,
  STATE_UNKNOWN,
  STATE_DEPENDENT
};

const char *state_text (int);
