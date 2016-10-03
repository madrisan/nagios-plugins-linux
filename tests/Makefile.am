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

abs_builddir = $(shell pwd)

test_clang_checker = tsclang_checker.sh 
check-clang-checker: 
	$(top_builddir)/tests/$(test_clang_checker)

AM_CPPFLAGS = \
        -Dabs_builddir="\"$(abs_builddir)\"" \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include

test_programs = \
	tsclock \
	tscswch \
	tsintr  \
        tsmessages \
        tskernelver

test_libraries = tslib_uname.la

test_utils = \
	$(top_srcdir)/include/testutils.h \
	testutils.c

LDADDS = $(top_builddir)/lib/libutils.a
TSLIBS_LDFLAGS = -module -avoid-version \
	-rpath /evil/libtool/hack/to/force/shared/lib/creation

tsclock_SOURCES = $(test_utils) tsclock.c
tsclock_LDADD   = $(LDADDS)

tscswch_SOURCES = $(test_utils) tscswch.c
tscswch_LDADD   = $(LDADDS)

tsintr_SOURCES  = $(test_utils) tsintr.c
tsintr_LDADD    = $(LDADDS)

tsmessages_SOURCES = $(test_utils) tsmessages.c
tsmessages_LDADD   = $(LDADDS)

tskernelver_SOURCES = $(test_utils) tskernelver.c
tskernelver_LDADD   = $(LDADDS)

tslib_uname_la_SOURCES = tslib_uname.c
tslib_uname_la_LDFLAGS = $(TSLIBS_LDFLAGS)

if WITH_TESTS
noinst_PROGRAMS = $(test_programs)
noinst_LTLIBRARIES = $(test_libraries)
else ! WITH_TESTS
check_PROGRAMS = $(test_programs)
check_LTLIBRARIES = $(test_libraries)
endif ! WITH_TESTS

TESTS = $(test_programs)
