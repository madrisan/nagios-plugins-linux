// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2024 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking SELinux status.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stddef.h>

#include "messages.h"
#include "mountlist.h"
#include "npl_selinux.h"
#include "sysfsparser.h"
#include "system.h"

char *selinux_mnt = NULL;

/* Return 1 if selinuxfs exists as a kernel filesystem, or 0 otherwise. */
static int
selinuxfs_exists ()
{
  int exists;

  exists = file_system_type_exists (SELINUXFS, &selinux_mnt);
  return (exists > 0) ? 1 : 0;
}

/* Return 2 if we are running on a SELinux kernel in enhanced mode,
 * 1 if SELinux is running in permissive mode, 0 otherwise.
 */
int
is_selinux_enabled (void)
{
  int fs_exists = selinuxfs_exists ();
  unsigned long long value;

  if (0 == fs_exists)
    return 0;
  if (0 == sysfsparser_path_exist ("%s/enforce", selinux_mnt))
    return 0;

  if ((sysfsparser_getvalue (&value, "%s/enforce", selinux_mnt) < 0))
    plugin_error (STATE_UNKNOWN, 0,
		  "cannot read the SELinux status from %s/enforce",
		  selinux_mnt);

  return value + 1;
}
