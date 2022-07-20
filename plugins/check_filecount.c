// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2022 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that returns the number of files found in one or more
 * directories.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "files.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "xstrton.h"

static const char *program_copyright =
  "Copyright (C) 2022 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "ignore-symlinks", no_argument, NULL, 'l'},
  {(char *) "ignore-unknown", no_argument, NULL, 'u'},
  {(char *) "include-hidden", no_argument, NULL, 'H'},
  {(char *) "name", required_argument, NULL, 'n'},
  {(char *) "recursive", no_argument, NULL, 'r'},
  {(char *) "regular-only", no_argument, NULL, 'f'},
  {(char *) "size", required_argument, NULL, 's'},
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
  fputs ("This plugin returns the number of files found in one or more "
	 "directories.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out,
	   "  %s [-w COUNTER] [-c COUNTER] [-f] [-H] [-l] [-r] [-u] \\\n"
	   "\t[-s SIZE] [-t AGE] [-n PATTERN] DIR [DIR...]\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -f, --regular-only       count regular files only\n", out);
  fputs ("  -H, --include-hidden     do not skip the hidden files\n", out);
  fputs ("  -l, --ignore-symlinks    ignore symlinks\n", out);
  fputs ("  -n, --name               only count files that match PATTERN\n",
	 out);
  fputs ("  -r, --recursive          check recursively each subdirectory\n",
	 out);
  fputs ("  -s, --size               count only files of a specific size\n",
	 out);
  fputs ("  -t, --time               count only files of a specific age\n",
	 out);
  fputs ("  -u, --ignore-unknown     ignore file with type unknown\n", out);
  fputs ("  -w, --warning COUNTER    warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_NOTE, out);
  fputs ("  By default, this plugin counts the directories and files"
	 " (of every type)\n"
	 "  found in DIR, in a non recursive way."
	 "  The hidden files are ignored.\n",
	 out);
  fputs ("  Option \"name\".\n"
	 "    Only count files that match PATTERN, where PATTERN is a shell-like"
	 " wildcard\n"
	 "    as understood by fnmatch(3).  Only the filename is checked against"
	 " the\n"
	 "    pattern, not the entire path.\n",
	 out);
  fputs ("  Option \"size\".\n"
	 "    When SIZE is a positive number, only files that are at least"
	 " this big\n"
	 "    are counted.  If SIZE is a negative number, this is inversed,"
	 " i. e.\n"
	 "    only files smaller than the absolute value of SIZE are counted.\n"
	 "    A \"multiplier\" may be added: b (byte), k (kilobyte),"
	 " m (megabyte),\n"
	 "    g (gigabyte), t (terabyte), and p (petabyte).  Please note that\n"
	 "    there are 1000 bytes in a kilobyte, not 1024.\n",
	 out);
  fputs ("  Option \"time\".\n"
         "    If Age is greater than zero, only files that haven't been"
	 " touched in the\n"
         "    last Age seconds are counted.  If Age is a negative number, this"
	 " is\n"
         "    inversed.  The number can also be followed by a \"multiplier\" to"
	 " easily\n"
	 "    specify a larger timespan.  Valid multipliers are s (second),"
	 " m (minute),\n"
	 "    h (hour), d (day), w (week), and y (year).\n",
	 out);

  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -l -r /tmp\n", program_name);
  fprintf (out, "  %s -w 150 -c 200 -f -r /var/log/myapp /tmp/myapp\n",
	   program_name);
  fprintf (out, "  %s -w 10 -c 15 -f -r -s -10.5k /tmp/myapp\n",
	   program_name);
  fprintf (out, "  %s -r -t -1h /tmp/myapp   # files modified in the last"
	   " hour\n", program_name);
  fprintf (out, "  %s -f -n \"myapp-202207*.log\" /var/log/myapp\n", program_name);

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
  int c, i, ret;
  bool verbose = false;
  char *bp, *critical = NULL, *warning = NULL,
       *errmesg_fage = NULL, *errmesg_fsize = NULL,
       *pattern = NULL;
  int64_t fileage = 0, filesize = 0;
  size_t size;
  unsigned int filecount_flags = FILES_DEFAULT;
  FILE *perfdata;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "c:fHln:rs:t:uvw:" GETOPT_HELP_VERSION_STRING,
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
	  filecount_flags |= FILES_REGULAR_ONLY;
	  break;
	case 'H':
	  filecount_flags |= FILES_INCLUDE_HIDDEN;
	  break;
	case 'l':
	  filecount_flags |= FILES_IGNORE_SYMLINKS;
	  break;
	case 'n':
	  pattern = optarg;
	  break;
	case 'r':
	  filecount_flags |= FILES_RECURSIVE;
	  break;
	case 's':
	  ret = sizetoint64 (optarg, &filesize, &errmesg_fsize);
	  if (ret < 0)
	    plugin_error (STATE_UNKNOWN, errno
			  , "failed to parse file size argument: %s"
			  , errmesg_fsize);
	  break;
	case 't':
	  ret = agetoint64 (optarg, &fileage, &errmesg_fage);
	  if (ret < 0)
	    plugin_error (STATE_UNKNOWN, errno
			  , "failed to parse file size argument: %s"
			  , errmesg_fage);
	  break;
	case 'u':
	  filecount_flags |= FILES_IGNORE_UNKNOWN;
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

      int partial = files_filecount (argv[i], filecount_flags,
				     fileage, filesize, pattern);
      if (partial < 0)
	plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", argv[i]);

      fprintf (perfdata, "%s_total=%d ", argv[i], partial);
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
