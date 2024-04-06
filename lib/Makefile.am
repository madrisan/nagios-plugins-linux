## Process this file with automake to produce Makefile.in

## Copyright (c) 2014-2024 Davide Madrisan <davide.madrisan@gmail.com>
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
	-I$(top_srcdir)/include \
	-DDOCKER_SOCKET=\"$(DOCKER_SOCKET)\" \
	-DPODMAN_SOCKET=\"$(PODMAN_SOCKET)\" \
	$(LIBCURL_CPPFLAGS) \
	$(LIBPROCPS_CPPFLAGS)

noinst_LIBRARIES = libutils.a

libutils_a_SOURCES =  \
	collection.c  \
	cpudesc.c     \
	cpufreq.c     \
	cpustats.c    \
	cputopology.c \
	files.c       \
	kernelver.c   \
	interrupts.c  \
	json_helpers.c \
	messages.c    \
	mountlist.c   \
	netinfo.c     \
	netinfo-private.c \
	npl_selinux.c \
	perfdata.c    \
	pressure.c    \
	processes.c   \
	procparser.c  \
	progname.c    \
	sysfsparser.c \
	thresholds.c  \
	tcpinfo.c     \
	url_encode.c  \
	xasprintf.c   \
	xmalloc.c     \
	xstrton.c

if HAVE_LIBCURL
  libutils_a_SOURCES += \
	container.c
endif

if HAVE_LIBPROCPS

  libutils_a_SOURCES += \
	meminfo_procps.c \
	vminfo_procps.c

else

if HAVE_PROC_MEMINFO
  libutils_a_SOURCES += meminfo.c
endif

  libutils_a_SOURCES += vminfo.c

endif
