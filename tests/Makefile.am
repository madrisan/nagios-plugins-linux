## Process this file with automake to produce Makefile.in

## Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include

test_programs = \
	tsclock \
	tscswch \
	tsintr

test_utils = testutils.h testutils.c

LDADDS = $(top_builddir)/lib/libutils.a

tsclock_SOURCES = $(test_utils) tsclock.c
tsclock_LDADD   = $(LDADDS)

tscswch_SOURCES = $(test_utils) tscswch.c
tscswch_LDADD   = $(LDADDS)

tsintr_SOURCES  = $(test_utils) tsintr.c
tsintr_LDADD    = $(LDADDS)

noinst_PROGRAMS = $(test_programs)
TESTS = $(test_programs)
