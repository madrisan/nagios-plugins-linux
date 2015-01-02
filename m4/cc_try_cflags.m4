dnl This file is part of `nagios-plugins-linux'.
dnl Copyright (C) 2014 by Davide Madrisan <davide.madrisan@gmail.com>

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Process this file with autoconf to produce a configure script.

dnl cc_TRY_CFLAGS (CFLAGS)
dnl ------------------------------------------------------------
dnl Checks if $CC supports a given set of CFLAGS.
dnl If supported, the current CFLAGS is appended to SUPPORTED_CFLAGS
AC_DEFUN([cc_TRY_CFLAGS],
   [AC_MSG_CHECKING([whether compiler accepts $1])
   ac_save_CFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS $1"
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[]],[[int x;]])],
     [AC_MSG_RESULT([yes])
      SUPPORTED_CFLAGS="$SUPPORTED_CFLAGS $1"],
     [AC_MSG_RESULT([no])]
   )
])
