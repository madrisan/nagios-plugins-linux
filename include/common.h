// SPDX-License-Identifier: GPL-3.0-or-later
/* common.h -- set of common macros and definitions

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _COMMON_H_
# define _COMMON_H_	1

#define GPLv3_DISCLAIMER \
  "\
License (SPDX ID): GPL-3.0-or-later \
<https://www.gnu.org/licenses/gpl.html>\n\
This is free software; you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"

#define GETOPT_HELP_CHAR 'h'
#define GETOPT_VERSION_CHAR 'V'
#define GETOPT_HELP_VERSION_STRING "hV"

#define USAGE_HEADER     "Usage:\n"
#define USAGE_OPTIONS    "Options:\n"
#define USAGE_EXAMPLES   "Examples:\n"
#define USAGE_NOTE       "Note:\n"
#define USAGE_NOTE_1     "Note 1:\n"
#define USAGE_NOTE_2     "Note 2:\n"
#define USAGE_SEPARATOR  "\n"
#define USAGE_HELP \
  "  -h, --help      display this help and exit\n"
#define USAGE_VERSION \
  "  -V, --version   output version information and exit\n"
#define USAGE_THRESHOLDS \
  "\
See the Nagios Developer Guidelines for range format:\n\
<https://nagios-plugins.org/doc/guidelines.html#THRESHOLDFORMAT>\n"

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR

#define case_GETOPT_HELP_CHAR                   \
  case GETOPT_HELP_CHAR:                        \
    usage (stdout);                             \
    break;

#define case_GETOPT_VERSION_CHAR                \
  case GETOPT_VERSION_CHAR:                     \
    print_version ();                           \
    printf (GPLv3_DISCLAIMER);                  \
    exit (EXIT_SUCCESS);                        \
    break;

/* One iteration with 1sec delay by default,
   for plugins that support iterations */
#define DELAY_DEFAULT   1
#define COUNT_DEFAULT   2
#define DELAY_MAX      60
#define COUNT_MAX     100

/* Nagios Plugins error codes */

typedef enum nagstatus
{
  STATE_OK,
  STATE_WARNING,
  STATE_CRITICAL,
  STATE_UNKNOWN,
  STATE_DEPENDENT
} nagstatus;

#endif	/* _COMMON_H_ */
