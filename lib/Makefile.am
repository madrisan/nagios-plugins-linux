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

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include

noinst_LIBRARIES = libutils.a

libutils_a_SOURCES = \
	cpudesc.c    \
	cpustats.c   \
	meminfo.c    \
	messages.c   \
	mountlist.c  \
	netinfo.c    \
	processes.c  \
	procparser.c \
	progname.c   \
	thresholds.c \
	tcpinfo.c    \
	vminfo.c     \
	xasprintf.c  \
	xmalloc.c
