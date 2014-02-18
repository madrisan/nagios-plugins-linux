#ifndef COMMON_H_
# define COMMON_H_

#define GPLv3_DISCLAIMER \
  "\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software; you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"

#define GETOPT_HELP_CHAR 'h'
#define GETOPT_VERSION_CHAR 'v'

#define HELP_OPTION_DESCRIPTION \
  "      --help      display this help and exit\n"
#define VERSION_OPTION_DESCRIPTION \
  "      --version   output version information and exit\n"

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

#endif
