/*
 * License: GPLv3+
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
#include "messages.h"
#include "mountlist.h"
#include "progname.h"
#include "progversion.h"

#define STREQ(a, b) (strcmp (a, b) == 0)

static const char *program_copyright =
  "Copyright (C) 2013-2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

/* Linked list of mounted file systems. */
static struct mount_entry *mount_list;

static struct option const longopts[] = {
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static void attribute_noreturn
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks whether the given filesystems are mounted.\n",
	 out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [FILESYSTEM]...\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s /mnt/nfs-data /mnt/cdrom\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void attribute_noreturn
print_version (void)
{
  printf ("%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs (program_copyright, stdout);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
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
  int c, status;  

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, GETOPT_HELP_VERSION_STRING, longopts,
                           NULL)) != -1)
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
                  "cannot read table of mounted file systems");

  status = STATE_OK;

  if (optind < argc)
    {
      int i;
      for (i = optind; i < argc; ++i)
	if (check_entry (argv[i]) == STATE_CRITICAL)
	  {
	    printf ("%s %s",
                    (status == STATE_OK) ? "FILESYSTEMS CRITICAL:" : ",",
                    argv[i]);
	    status = STATE_CRITICAL;
	  }
    }
  else
    usage (stderr);

  if (status == STATE_OK)
    printf ("FILESYSTEMS OK\n");
  else
    printf (" not mounted!\n");

  return status;
}
