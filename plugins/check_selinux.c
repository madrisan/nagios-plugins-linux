// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2024 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that checks if SELinux is enabled.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "messages.h"
#include "npl_selinux.h"
#include "progname.h"
#include "progversion.h"
#include "system.h"
#include "xasprintf.h"

extern char *selinux_mnt;

static const char *program_copyright =
  "Copyright (C) 2024 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs ("This plugin checks if SELinux is enabled.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s -w COUNTER|PERC -c COUNTER|PERC\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s ...\n",
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
  int c, is_enabled;
  nagstatus status = STATE_OK;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  is_enabled = is_selinux_enabled ();
  status = is_enabled ? STATE_OK : STATE_CRITICAL;

  printf ("%s %s - selinux %s%s | selinux_enabled=%d\n"
	  , program_name_short
	  , state_text (status)
	  , status == STATE_OK ? "enabled" : "disabled"
	  , selinux_mnt ? xasprintf (" (%s)", selinux_mnt) : ""
	  , is_enabled);

  return status;
}
