/*
 * License: Public Domain
 * Copyright (c) Fred Bulback
 * Source Code: http://www.geekhideout.com/urlcode.shtml
 *
 * A library for encoding URLs.
 *
 * To the extent possible under law, Fred Bulback has waived all copyright
 * and related or neighboring rights to URL Encoding/Decoding in C.
 * This work is published from: United States.  */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Converts a hex character to its integer value */
char
from_hex (char ch)
{
  return isdigit (ch) ? ch - '0' : tolower (ch) - 'a' + 10;
}

/* Converts an integer value to its hex character */
char
to_hex (char code)
{
  const char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *
url_encode (char *str)
{
  char *pstr = str, *buf = malloc (strlen (str) * 3 + 1), *pbuf = buf;
  while (*pstr)
    {
      if (isalnum (*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.'
	  || *pstr == '~')
	*pbuf++ = *pstr;
      else if (*pstr == ' ')
	*pbuf++ = '+';
      else
	{
	  *pbuf++ = '%';
	  *pbuf++ = to_hex (*pstr >> 4);
	  *pbuf++ = to_hex (*pstr & 15);
	}
      pstr++;
    }
  *pbuf = '\0';
  return buf;
}
