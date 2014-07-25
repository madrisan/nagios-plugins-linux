#ifndef _COMMON_H_
# define _COMMON_H_	1

#define GPLv3_DISCLAIMER \
  "\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software; you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"

#define GETOPT_HELP_CHAR 'h'
#define GETOPT_VERSION_CHAR 'V'
#define GETOPT_HELP_VERSION_STRING "hV"

#define USAGE_HEADER     "Usage:\n"
#define USAGE_OPTIONS    "Options:\n"
#define USAGE_EXAMPLES   "Examples:\n"
#define USAGE_NOTE       "Note:\n"
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
