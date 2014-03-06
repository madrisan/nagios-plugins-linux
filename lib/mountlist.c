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

/* This library was largely based on and inspired by the coreutils code.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mountlist.h"
#include "xalloc.h"

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <mntent.h>
#if !defined MOUNTED
# if defined _PATH_MOUNTED	/* GNU libc  */
#  define MOUNTED _PATH_MOUNTED
# endif
#endif

#ifndef ME_DUMMY
# define ME_DUMMY(Fs_name, Fs_type)             \
    (strcmp (Fs_type, "autofs") == 0            \
     || strcmp (Fs_type, "proc") == 0           \
   /* for Linux 2.6/3.x */                      \
     || strcmp (Fs_type, "cgroup") == 0         \
     || strcmp (Fs_type, "debugfs") == 0        \
     || strcmp (Fs_type, "devpts") == 0         \
     || strcmp (Fs_type, "fusectl") == 0        \
     || strcmp (Fs_type, "hugetlbfs") == 0      \
     || strcmp (Fs_type, "mqueue") == 0         \
     || strcmp (Fs_type, "pstore") == 0         \
     || strcmp (Fs_type, "rpc_pipefs") == 0     \
     || strcmp (Fs_type, "securityfs") == 0     \
     || strcmp (Fs_type, "sysfs") == 0          \
   /* Linux 2.4 */                              \
     || strcmp (Fs_type, "devfs") == 0          \
     || strcmp (Fs_type, "binfmt_misc") == 0    \
     || strcmp (Fs_type, "none") == 0)
#endif

#ifndef ME_REMOTE
/* A file system is "remote" if its Fs_name contains a ':'
 *    or if (it is of type (smbfs or cifs) and its Fs_name starts with '//').  */
# define ME_REMOTE(Fs_name, Fs_type)            \
    (strchr (Fs_name, ':') != NULL              \
     || ((Fs_name)[0] == '/'                    \
         && (Fs_name)[1] == '/'                 \
         && (strcmp (Fs_type, "smbfs") == 0     \
             || strcmp (Fs_type, "cifs") == 0)))
#endif

/* Check for the "ro" pattern in the MOUNT_OPTIONS.
 *    Return true if found, Otherwise return false.  */
static bool
fs_check_if_readonly (char *mount_options)
{
  static char const readonly_pattern[] = "ro";
  char *str1, *token, *saveptr1;
  int j;

  for (j = 1, str1 = mount_options;; j++, str1 = NULL)
    {
      token = strtok_r (str1, ",", &saveptr1);
      if (token == NULL)
	break;
      if (strcmp (token, readonly_pattern) == 0)
	return true;
    }

  return false;
}

/* Return the device number from MOUNT_OPTIONS, if possible.
 *    Otherwise return (dev_t) -1.  */
static dev_t
dev_from_mount_options (char const *mount_options)
{
  /* GNU/Linux allows file system implementations to define their own
   *      meaning for "dev=" mount options, so don't trust the meaning
   *           here.  */
  (void) mount_options;
  return -1;
}

/* Return a list of the currently mounted file systems, or NULL on error.
   Add each entry to the tail of the list so that they stay in order.
   If NEED_FS_TYPE is true, ensure that the file system type fields in
   the returned list are valid.  Otherwise, they might not be.  */

struct mount_entry *
read_file_system_list (bool need_fs_type)
{
  struct mount_entry *mount_list;
  struct mount_entry *me;
  struct mount_entry **mtail = &mount_list;
  (void) need_fs_type;

  {
    struct mntent *mnt;
    char const *table = MOUNTED;
    FILE *fp;

    fp = setmntent (table, "r");
    if (fp == NULL)
      return NULL;

    while ((mnt = getmntent (fp)))
      {
	me = xmalloc (sizeof *me);
	me->me_devname = xstrdup (mnt->mnt_fsname);
	me->me_mountdir = xstrdup (mnt->mnt_dir);
	me->me_type = xstrdup (mnt->mnt_type);
	me->me_type_malloced = 1;
	me->me_opts = xstrdup (mnt->mnt_opts);
	me->me_opts_malloced = 1;
	me->me_dummy = ME_DUMMY (me->me_devname, me->me_type);
	me->me_remote = ME_REMOTE (me->me_devname, me->me_type);
	me->me_readonly = fs_check_if_readonly (me->me_opts);
	me->me_dev = dev_from_mount_options (mnt->mnt_opts);

	/* Add to the linked list. */
	*mtail = me;
	mtail = &me->me_next;
      }

    if (endmntent (fp) == 0)
      goto free_then_fail;
  }

  *mtail = NULL;
  return mount_list;


free_then_fail:
  {
    int saved_errno = errno;
    *mtail = NULL;

    while (mount_list)
      {
	me = mount_list->me_next;
	free (mount_list->me_devname);
	free (mount_list->me_mountdir);
	if (mount_list->me_type_malloced)
	  free (mount_list->me_type);
	if (mount_list->me_opts_malloced)
	  free (mount_list->me_opts);
	free (mount_list);
	mount_list = me;
      }

    errno = saved_errno;
    return NULL;
  }
}
