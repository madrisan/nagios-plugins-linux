## Process this file with automake to produce Makefile.in

## Copyright (c) 2016-2022 Davide Madrisan <davide.madrisan@gmail.com>
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

AM_CFLAGS = $(LIBPROCPS_CFLAGS)
AM_LDFLAGS = $(LIBPROCPS_LIBS)

test_programs = \
	tslibcontainer_docker_count \
	tslibcontainer_docker_memory \
	tslibfiles_age \
	tslibfiles_filecount \
	tslibfiles_hiddenfile \
	tslibfiles_size \
	tslibkernelver \
	tslibmeminfo_conversions \
	tslibmeminfo_interface \
	tslibmeminfo_procparser \
	tslibmessages \
	tslibperfdata \
	tslibpressure \
	tsliburlencode \
	tslibxstrton_agetoint64 \
	tslibxstrton_sizetoint64
if !HAVE_LIBPROCPS
test_programs += \
	tslibvminfo
endif
test_programs += \
	tsclock_thresholds \
	tscswch \
	tsintr  \
	tsload_normalize \
	tsload_thresholds \
	tspaging \
	tstestutils \
	tsuptime \
	tstemperature

test_libraries = tslibuname.la

test_utils = \
	$(top_srcdir)/include/testutils.h \
	testutils.c

LDADDS = $(top_builddir)/lib/libutils.a
TSLIBS_LDFLAGS = -module -avoid-version \
	-rpath /evil/libtool/hack/to/force/shared/lib/creation

tslibcontainer_docker_count_SOURCES = $(test_utils) tslibcontainer_docker_count.c
tslibcontainer_docker_count_LDADD = $(LDADDS)
tslibcontainer_docker_memory_SOURCES = $(test_utils) tslibcontainer_docker_memory.c
tslibcontainer_docker_memory_LDADD = $(LDADDS)

tslibfiles_age_SOURCES = $(test_utils) tslibfiles_age.c
tslibfiles_age_LDADD = $(LDADDS)
tslibfiles_filecount_SOURCES = $(test_utils) tslibfiles_filecount.c
tslibfiles_filecount_LDADD = $(LDADDS)
tslibfiles_hiddenfile_SOURCES = $(test_utils) tslibfiles_hiddenfile.c
tslibfiles_hiddenfile_LDADD = $(LDADDS)
tslibfiles_size_SOURCES = $(test_utils) tslibfiles_size.c
tslibfiles_size_LDADD = $(LDADDS)

tslibkernelver_SOURCES = $(test_utils) tslibkernelver.c
tslibkernelver_LDADD = $(LDADDS)

tslibmeminfo_conversions_SOURCES = $(test_utils) tslibmeminfo_conversions.c
tslibmeminfo_conversions_LDADD = $(LDADDS)
tslibmeminfo_interface_SOURCES = $(test_utils) tslibmeminfo_interface.c
tslibmeminfo_interface_LDADD = $(LDADDS)
tslibmeminfo_procparser_SOURCES = $(test_utils) tslibmeminfo_procparser.c
tslibmeminfo_procparser_LDADD = $(LDADDS)

tslibmessages_SOURCES = $(test_utils) tslibmessages.c
tslibmessages_LDADD = $(LDADDS)

tslibperfdata_SOURCES = $(test_utils) tslibperfdata.c
tslibperfdata_LDADD = $(LDADDS)

tslibpressure_SOURCES = $(test_utils) tslibpressure.c
tslibpressure_LDADD = $(LDADDS)

tsliburlencode_SOURCES = $(test_utils) tsliburlencode.c
tsliburlencode_LDADD = $(LDADDS)

tslibxstrton_agetoint64_SOURCES = $(test_utils) tslibxstrton_agetoint64.c
tslibxstrton_agetoint64_LDADD = $(LDADDS)
tslibxstrton_sizetoint64_SOURCES = $(test_utils) tslibxstrton_sizetoint64.c
tslibxstrton_sizetoint64_LDADD = $(LDADDS)

tslibvminfo_SOURCES = $(test_utils) tslibvminfo.c
tslibvminfo_LDADD = $(LDADDS)

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

tspaging_SOURCES = $(test_utils) tspaging.c
tspaging_LDADD = $(LDADDS)

tsuptime_SOURCES = $(test_utils) tsuptime.c
tsuptime_LDADD = $(LDADDS)

tstemperature_SOURCES = $(test_utils) tstemperature.c
tstemperature_LDADD = $(LDADDS)

tstestutils_SOURCES = $(test_utils) tstestutils.c
tstestutils_LDADD = $(LDADDS)

tslibuname_la_SOURCES = tslibuname.c
tslibuname_la_LDFLAGS = $(TSLIBS_LDFLAGS)

if WITH_TESTS
noinst_PROGRAMS = $(test_programs)
noinst_LTLIBRARIES = $(test_libraries)
else ! WITH_TESTS
check_PROGRAMS = $(test_programs)
check_LTLIBRARIES = $(test_libraries)
endif ! WITH_TESTS

TESTS = $(test_programs)

dist_noinst_DATA = \
	ts_container_docker.data \
	ts_container_podman_GetContainerStats.data \
	ts_container_podman_ListContainers.data \
	ts_procmeminfo.data \
	ts_procpressurecpu.data \
	ts_procpressureio.data \
	ts_procstat.data \
	ts_procvmstat.data \
	ts_sysdockermemstat.data
