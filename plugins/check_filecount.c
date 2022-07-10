// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that do something ....
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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "files.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"

static const char *program_copyright =
  "Copyright (C) 2022 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "ignore-symlinks", required_argument, NULL, 'l'},
  {(char *) "ignore-unknown", required_argument, NULL, 'u'},
  {(char *) "recursive", required_argument, NULL, 'r'},
  {(char *) "regular-files", required_argument, NULL, 'f'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "verbose", no_argument, NULL, 'v'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks the number of files in the given directories.\n",
	 out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out,
	   "  %s [-w COUNTER] [-c COUNTER DIR] [-f] [-l] [-r] [-u] "
	   "DIR [DIR...]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -f, --regular-files      count regular files only\n", out);
  fputs ("  -l, --ignore-symlinks    ignore symlinks\n", out);
  fputs ("  -r, --recursive          check recursively each subdirectory\n",
	 out);
  fputs ("  -u, --ignore-unknown     ignore file with type unknown\n", out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_NOTE, out);
  fputs ("  By default, this plugin counts files and directories of DIR not"
	 " recursively.\n", out);
  fputs ("  If the filesystem does not provide the type of files and the"
	 " --ignore-unknown\n"
	 "  option has been selected, the file number will be zero."
         "  If this option is not\n"
	 "  specified, any file type restriction will be silently ignored"
	 " and all files\n"
	 "  counted.  Only some filesystems (among them: Btrfs, ext2, ext3,"
	 " and ext4) have\n"
	 "  full support for returning file types."
	 "  See the readdir(3) manpage.\n",
	 out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -l -r /tmp\n", program_name);
  fprintf (out, "  %s -w 150 -c 200 -f -r /var/log/myapp /tmp/myapp\n",
	   program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static _Noreturn void
print_version (void)
{
  printf ("%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs (program_copyright, stdout);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
}

int
main (int argc, char **argv)
{
  int c, i;
  bool verbose = false;
  char *bp, *critical = NULL, *warning = NULL;
  size_t size;
  unsigned int filecount_flags = READDIR_DEFAULT;
  FILE *perfdata;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:flruvw:" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'c':
	  critical = optarg;
	  break;
	case 'f':
	  filecount_flags |= READDIR_REGULAR_FILES_ONLY;
	  break;
	case 'l':
	  filecount_flags |= READDIR_IGNORE_SYMLINKS;
	  break;
	case 'r':
	  filecount_flags |= READDIR_RECURSIVE;
	  break;
	case 'u':
	  filecount_flags |= READDIR_IGNORE_UNKNOWN;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	case 'v':
	  verbose = true;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  if (argc <= optind)
    usage (stderr);

  int filecount = 0;
  perfdata = open_memstream (&bp, &size);

  for (i = optind; i < argc; ++i)
    {
      if (verbose)
	printf("checking directory %s with flags %u ...\n", argv[i],
	       filecount_flags);

      int partial = files_filecount(argv[i], filecount_flags);
      if (partial < 0)
	plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", argv[i]);

      fprintf (perfdata, "%s=%d ", argv[i], partial);
      filecount += partial;
    }

  fclose (perfdata);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  status = get_status (filecount, my_threshold);
  free (my_threshold);

  printf ("%s %s - total number of files: %d | %s\n",
	  program_name_short, state_text (status), filecount, bp);

  return status;
}
