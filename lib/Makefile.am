## Process this file with automake to produce Makefile.in

## Copyright (C) 2003 Davide Madrisan <davide.madrisan@gmail.com>

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CPPFLAGS = -I$(top_srcdir)/src

noinst_HEADERS =  \
  common.h        \
  error.h         \
  mountlist.h     \
  xalloc.h        \
  nputils.h

noinst_LIBRARIES = libfilesystems.a libnp.a

libfilesystems_a_SOURCES = \
  error.c                  \
  mountlist.c              \
  xmalloc.c

libnp_a_SOURCES = nputils.c nputils.h

libfilesystems_a_LIBADD = $(LIBOBJS)




