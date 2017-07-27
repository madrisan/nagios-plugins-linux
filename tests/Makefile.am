## Process this file with automake to produce Makefile.in

## Copyright (c) 2016,2017 Davide Madrisan <davide.madrisan@gmail.com>
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

#abs_builddir = $(shell pwd)

SCAN_BUILD_BINARY = /usr/bin/scan-build
SCAN_BUILD_COMMAND = make clean all
SCAN_BUILD_OUTDIR = /tmp/clang-checker-analysis
SCAN_BUILD_OPTS = \
	-analyze-headers \
	-enable-checker alpha.deadcode.UnreachableCode \
	-enable-checker alpha.security.ArrayBoundV2 \
	-enable-checker alpha.security.taint.TaintPropagation \
	-enable-checker nullability.NullableDereferenced \
	-enable-checker security.insecureAPI.strcpy \
	-o $(SCAN_BUILD_OUTDIR)

check-clang-checker:
	test -x $(SCAN_BUILD_BINARY) || \
	  { echo 'WARNING: scan-build not found' >&2; exit 77; }; \
	$(SCAN_BUILD_BINARY) \
	  $(SCAN_BUILD_OPTS) $(SCAN_BUILD_COMMAND)

AM_CPPFLAGS = \
	-Dabs_builddir="\"$(abs_builddir)\"" \
	-Dabs_srcdir="\"$(abs_srcdir)\"" \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include

test_programs = \
	tslibmeminfo \
	tsclock_thresholds \
	tscswch \
	tsintr  \
	tsload_normalize \
	tsload_thresholds \
	tsuptime \
	tsmessages \
	tsmemory_conversions \
	tsmemory_meminfo \
	tsmemory_vmstat \
	tstemperature \
	tskernelver

test_libraries = tslib_uname.la

test_utils = \
	$(top_srcdir)/include/testutils.h \
	testutils.c

LDADDS = $(top_builddir)/lib/libutils.a
TSLIBS_LDFLAGS = -module -avoid-version \
	-rpath /evil/libtool/hack/to/force/shared/lib/creation

tslibmeminfo_SOURCES = $(test_utils) tslibmeminfo.c
tslibmeminfo_LDADD = $(LDADDS)

tsclock_thresholds_SOURCES = $(test_utils) tsclock_thresholds.c
tsclock_thresholds_LDADD = $(LDADDS)

tscswch_SOURCES = $(test_utils) tscswch.c
tscswch_LDADD = $(LDADDS)

tsintr_SOURCES = $(test_utils) tsintr.c
tsintr_LDADD = $(LDADDS)

tsload_normalize_SOURCES = $(test_utils) tsload_normalize.c
tsload_normalize_LDADD = $(LDADDS)

tsload_thresholds_SOURCES = $(test_utils) tsload_thresholds.c
tsload_thresholds_LDADD = $(LDADDS)

tsmemory_conversions_SOURCES = $(test_utils) tsmemory_conversions.c
tsmemory_conversions_LDADD = $(LDADDS)

tsmemory_meminfo_SOURCES = $(test_utils) tsmemory_meminfo.c
tsmemory_meminfo_LDADD = $(LDADDS)

tsmemory_vmstat_SOURCES = $(test_utils) tsmemory_vmstat.c
tsmemory_vmstat_LDADD = $(LDADDS)

tsuptime_SOURCES = $(test_utils) tsuptime.c
tsuptime_LDADD = $(LDADDS)

tsmessages_SOURCES = $(test_utils) tsmessages.c
tsmessages_LDADD = $(LDADDS)

tstemperature_SOURCES = $(test_utils) tstemperature.c
tstemperature_LDADD = $(LDADDS)

tskernelver_SOURCES = $(test_utils) tskernelver.c
tskernelver_LDADD = $(LDADDS)

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

dist_noinst_DATA = \
	ts_procmeminfo.data \
	ts_procstat.data \
	ts_procvmstat.data
