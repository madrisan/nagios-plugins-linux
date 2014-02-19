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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "error.h"
#include "mountlist.h"
#include "nputils.h"
#include "xalloc.h"
#include "progname.h"

#define STREQ(a, b) (strcmp (a, b) == 0)

static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2013 Davide Madrisan <" PACKAGE_BUGREPORT ">";

/* A file system type to display. */

struct fs_type_list
{
  char *fs_name;
  struct fs_type_list *fs_next;
};

/* Linked list of file system types to display.
 * If 'fs_select_list' is NULL, list all types.
 * This table is generated dynamically from command-line options,
 * rather than hardcoding into the program what it thinks are the
 * valid file system types; let the user specify any file system type
 * they want to, and if there are any file systems of that type, they
 * will be shown.
 *
 * Some file system types:
 * 4.2 4.3 ufs nfs swap ignore io vm efs dbg */

static struct fs_type_list *fs_select_list;

/* Linked list of file system types to omit.
 *    If the list is empty, don't exclude any types.  */
static struct fs_type_list *fs_exclude_list;

/* Linked list of mounted file systems. */
static struct mount_entry *mount_list;

/* If true, show even file systems with zero size or
   uninteresting types. */
static bool show_all_fs;

/* If true, show only local file systems.  */
static bool show_local_fs;

/* If true, show each file system corresponding to the
   command line arguments.  */
static bool show_listed_fs;

static struct option const longopts[] = {
  {(char *) "all", no_argument, NULL, 'a'},
  {(char *) "local", no_argument, NULL, 'l'},
  {(char *) "list", no_argument, NULL, 'L'},
  {(char *) "type", required_argument, NULL, 'T'},
  {(char *) "exclude-type", required_argument, NULL, 'X'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

/* Add FSTYPE to the list of file system types to display. */

static void
add_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = xmalloc (sizeof *fsp);
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_select_list;
  fs_select_list = fsp;
}

/* Is FSTYPE a type of file system that should be listed?  */

static bool
selected_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_select_list == NULL || fstype == NULL)
    return true;
  for (fsp = fs_select_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return true;
  return false;
}

/* Add FSTYPE to the list of file system types to be omitted. */

static void
add_excluded_fs_type (const char *fstype)
{
  struct fs_type_list *fsp;

  fsp = xmalloc (sizeof *fsp);
  fsp->fs_name = (char *) fstype;
  fsp->fs_next = fs_exclude_list;
  fs_exclude_list = fsp;
}

/* Is FSTYPE a type of file system that should be omitted?  */

static bool
excluded_fstype (const char *fstype)
{
  const struct fs_type_list *fsp;

  if (fs_exclude_list == NULL || fstype == NULL)
    return false;
  for (fsp = fs_exclude_list; fsp; fsp = fsp->fs_next)
    if (STREQ (fstype, fsp->fs_name))
      return true;
  return false;
}

static bool
skip_mount_entry (struct mount_entry *me)
{
  if (me->me_remote && show_local_fs)
    return true;

  if (me->me_dummy && !show_all_fs)
    return true;

  if (!selected_fstype (me->me_type) || excluded_fstype (me->me_type))
    return true;

  return false;
}

static int
check_all_entries (void)
{
  struct mount_entry *me;
  int status = STATE_OK;

  for (me = mount_list; me; me = me->me_next)
    {
      if (skip_mount_entry (me))
	continue;

      if (show_listed_fs)
	printf ("%-10s %s type %s (%s) %s\n",
		me->me_devname, me->me_mountdir, me->me_type, me->me_opts,
		(me->me_readonly) ? "<< read-only" : "");
      else if (me->me_readonly && (status == STATE_OK))
	printf ("FILESYSTEMS CRITICAL: %s", me->me_mountdir);
      else if (me->me_readonly)
	printf (",%s", me->me_mountdir);

      if (me->me_readonly)
	status = STATE_CRITICAL;
    }

  return status;
}

static int
check_entry (char const *name)
{
  struct mount_entry *me;

  for (me = mount_list; me; me = me->me_next)
    if (STREQ (me->me_mountdir, name))
      {
	if (skip_mount_entry (me))
	  return STATE_OK;

	if (show_listed_fs)
	  printf ("%-10s %s type %s (%s) %s\n",
		  me->me_devname, me->me_mountdir, me->me_type, me->me_opts,
		  (me->me_readonly) ? "<< read-only" : "");

	if (me->me_readonly)
	  return STATE_CRITICAL;
      }

  return STATE_OK;
}

static void attribute_noreturn usage (FILE * out)
{
  fprintf (out, "%s, version %s - check for readonly filesystems.\n",
	   program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out, "Usage: %s [OPTION]... [FILESYSTEM]...\n\n", program_name);
  fputs ("\
  -a, --all                 include dummy file systems\n\
  -l, --local               limit listing to local file systems\n\
  -L, --list                display the list of checked file systems\n\
  -T, --type=TYPE           limit listing to file systems of type TYPE\n\
  -X, --exclude-type=TYPE   limit listing to file systems not of type TYPE\n", out);
  fputs (HELP_OPTION_DESCRIPTION, out);
  fputs (VERSION_OPTION_DESCRIPTION, out);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void
print_version (void)
{
  printf ("%s, version %s\n%s\n", program_name, program_version,
          program_copyright);
  fputs (GPLv3_DISCLAIMER, stdout);
}

int
main (int argc, char **argv)
{
  int c, status = STATE_OK;
  struct stat *stats = 0;

  fs_select_list = NULL;
  fs_exclude_list = NULL;

  show_local_fs = false;
  show_listed_fs = false;
  show_all_fs = false;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv, "alLT:X:hv", longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	  break;
	case 'a':
	  show_all_fs = true;
	  break;
	case 'l':
	  show_local_fs = true;
	  break;
	case 'L':
	  show_listed_fs = true;
	  break;
	case 'T':
	  add_fs_type (optarg);
	  break;
	case 'X':
	  add_excluded_fs_type (optarg);
	  break;

	case_GETOPT_HELP_CHAR
        case_GETOPT_VERSION_CHAR

	}
    }

  /* Fail if the same file system type was both selected and excluded.  */
  {
    struct fs_type_list *fs_incl;
    for (fs_incl = fs_select_list; fs_incl; fs_incl = fs_incl->fs_next)
      {
	struct fs_type_list *fs_excl;
	for (fs_excl = fs_exclude_list; fs_excl; fs_excl = fs_excl->fs_next)
	  {
	    if (STREQ (fs_incl->fs_name, fs_excl->fs_name))
	      plugin_error (STATE_UNKNOWN, 0,
		            "file system type `%s' both selected and "
                            "excluded",
		            fs_incl->fs_name);
	  }
      }
  }

  if (optind < argc)
    {
      int i;

      /* Open each of the given entries to make sure any corresponding
       * partition is automounted.  This must be done before reading the
       * file system table.  */
      stats = xnmalloc (argc - optind, sizeof *stats);
      for (i = optind; i < argc; ++i)
	{
	  /* Prefer to open with O_NOCTTY and use fstat, but fall back
	   * on using "stat", in case the file is unreadable.  */
	  int fd = open (argv[i], O_RDONLY | O_NOCTTY);
	  if ((fd < 0 || fstat (fd, &stats[i - optind]))
	      && stat (argv[i], &stats[i - optind]))
	    {
	      plugin_error (STATE_UNKNOWN, 0, "cannot open `%s'", argv[i]);
	      argv[i] = NULL;
	    }
	  if (0 <= fd)
	    close (fd);
	}
      free (stats);
    }

  mount_list =
    read_file_system_list ((fs_select_list != NULL
			    || fs_exclude_list != NULL || show_local_fs));

  if (NULL == mount_list)
    /* Couldn't read the table of mounted file systems. */
    plugin_error (STATE_UNKNOWN, 0,
                  "cannot read table of mounted file systems");

  if (optind < argc)
    {
      int i;

      for (i = optind; i < argc; ++i)
	if (argv[i] && (check_entry (argv[i]) == STATE_CRITICAL))
	  {
	    if (!show_listed_fs)
	      printf ("%s%s",
		      status == STATE_OK ? "FILESYSTEMS CRITICAL: " : ",",
		      argv[i]);
	    status = STATE_CRITICAL;
	  }
    }
  else
    status = check_all_entries ();

  if (!show_listed_fs)
    printf ("%s\n", (status == STATE_OK) ? "FILESYSTEMS OK" : " readonly!");

  return status;
}
