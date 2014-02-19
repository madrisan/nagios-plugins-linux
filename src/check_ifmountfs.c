/*
 * License: GPL
 * Copyright (c) 2013 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check for readonly filesystems
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
 */

#include "config.h"

#include <getopt.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "error.h"
#include "mountlist.h"
#include "nputils.h"
#include "progname.h"

#define STREQ(a, b) (strcmp (a, b) == 0)

static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2013 Davide Madrisan <" PACKAGE_BUGREPORT ">";

/* Linked list of mounted file systems. */
static struct mount_entry *mount_list;

static struct option const longopts[] = {
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static void attribute_noreturn usage (FILE * out)
{
  fprintf (out,
	   "%s, version %s - check whether the given filesystems are mounted.\n",
	   program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out, "Usage: %s [FILESYSTEM]...\n\n", program_name);
  fputs (HELP_OPTION_DESCRIPTION, out);
  fputs (VERSION_OPTION_DESCRIPTION, out);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void
print_version (void)
{
  printf ("%s, version %s\n%s\n", program_name, program_version,
	  program_copyright);
}

static int
check_entry (char const *mountpoint)
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    if (STREQ (me->me_mountdir, mountpoint))
      return STATE_OK;

  return STATE_CRITICAL;
}

int
main (int argc, char **argv)
{
  int c, status = STATE_OK;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "hv", longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  mount_list = read_file_system_list (false);

  if (NULL == mount_list)
    /* Couldn't read the table of mounted file systems. */
    plugin_error (STATE_UNKNOWN, 0,
                  "cannot read table of mounted file systems\n");

  if (optind < argc)
    {
      int i;
      for (i = optind; i < argc; ++i)
	if (check_entry (argv[i]) == STATE_CRITICAL)
	  {
	    printf ("FILESYSTEM CRITICAL: `%s' not mounted\n", argv[i]);
	    status = STATE_CRITICAL;
	  }
    }
  else
    usage (stderr);

  if (status == STATE_OK)
    printf ("FILESYSTEMS OK\n");

  return status;
}
