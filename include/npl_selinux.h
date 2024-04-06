// SPDX-License-Identifier: GPL-3.0-or-later
/* npl_selinux.h -- a library for getting informations about SELinux

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _NPL_SELINUX_H
#define _NPL_SELINUX_H        1

#define SELINUXMNT "/sys/fs/selinux"
#define OLDSELINUXMNT "/selinux"
#define SELINUXFS "selinuxfs"

/* Return 1 if we are running on a SELinux kernel, or 0 otherwise.
 * selinux_fs is set to the selinux filesystem mount point.
 */
int is_selinux_enabled (void);

#endif /* npl_selinux.h */
