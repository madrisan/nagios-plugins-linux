## Process this file with automake to produce Makefile.in

## Copyright (c) 2014-2016 Davide Madrisan <davide.madrisan@gmail.com>
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
	-DMULTIPATHD_SOCKET=\"$(MULTIPATHD_SOCKET)\" \
        -DVARLINK_ADDRESS=\"$(VARLINK_ADDRESS)\"

AM_CFLAGS = $(LIBPROCPS_CFLAGS)
AM_LDFLAGS = $(LIBPROCPS_LIBS)

LN_S=@LN_S@

check_cpu_programs = check_iowait

libexec_PROGRAMS =        \
	check_clock       \
	check_cpu         \
	check_cpufreq     \
	check_cswch       \
	check_fc          \
	check_ifmountfs   \
	check_intr        \
	check_multipath   \
	check_nbprocs     \
	check_network     \
	check_paging      \
	check_readonlyfs  \
	check_temperature \
	check_tcpcount    \
	check_uptime      \
	check_users

if HAVE_GETLOADAVG
libexec_PROGRAMS += \
	check_load
endif
if HAVE_LIBCURL
libexec_PROGRAMS += \
	check_docker
endif
if HAVE_PROC_MEMINFO
libexec_PROGRAMS += \
	check_memory      \
	check_swap
endif
if HAVE_LIBVARLINK
libexec_PROGRAMS += \
	check_podman
endif

check_clock_SOURCES      = check_clock.c
check_cpu_SOURCES        = check_cpu.c
check_cpufreq_SOURCES    = check_cpufreq.c
check_cswch_SOURCES      = check_cswch.c
check_fc_SOURCES         = check_fc.c
check_ifmountfs_SOURCES  = check_ifmountfs.c
check_intr_SOURCES       = check_intr.c
if HAVE_GETLOADAVG
check_load_SOURCES       = check_load.c
endif
if HAVE_LIBCURL
check_docker_SOURCES     = check_docker.c
endif
if HAVE_PROC_MEMINFO
check_memory_SOURCES     = check_memory.c
endif
check_multipath_SOURCES  = check_multipath.c
check_nbprocs_SOURCES    = check_nbprocs.c
check_network_SOURCES    = check_network.c
check_paging_SOURCES     = check_paging.c
if HAVE_LIBVARLINK
check_podman_SOURCES     = check_podman.c
endif
check_readonlyfs_SOURCES = check_readonlyfs.c
if HAVE_PROC_MEMINFO
check_swap_SOURCES       = check_swap.c
endif
check_tcpcount_SOURCES   = check_tcpcount.c
check_temperature_SOURCES = check_temperature.c
check_uptime_SOURCES     = check_uptime.c
check_users_SOURCES      = check_users.c

LDADD = $(top_builddir)/lib/libutils.a

check_clock_LDADD        = $(LDADD)
check_cpu_LDADD          = $(LDADD)
check_cpufreq_LDADD      = $(LDADD)
check_cswch_LDADD        = $(LDADD)
check_fc_LDADD           = $(LDADD)
check_ifmountfs_LDADD    = $(LDADD)
check_intr_LDADD         = $(LDADD)
if HAVE_GETLOADAVG
check_load_LDADD         = $(LDADD)
endif
if HAVE_LIBCURL
check_docker_LDADD       = $(LDADD) $(LIBCURL) -lm
endif
if HAVE_PROC_MEMINFO
check_memory_LDADD       = $(LDADD) $(LIBPROCPS_LIBS)
endif
check_nbprocs_LDADD      = $(LDADD)
check_network_LDADD      = $(LDADD) -lm
check_multipath_LDADD    = $(LDADD)
check_paging_LDADD       = $(LDADD) $(LIBPROCPS_LIBS)
if HAVE_LIBVARLINK
check_podman_LDADD       = $(LDADD) $(LIBVARLINK_LIBS)
endif
check_readonlyfs_LDADD   = $(LDADD)
if HAVE_PROC_MEMINFO
check_swap_LDADD         = $(LDADD)
endif
check_tcpcount_LDADD     = $(LDADD)
check_temperature_LDADD  = $(LDADD)
check_uptime_LDADD       = $(LDADD)
check_users_LDADD        = $(LDADD)

all-local: $(check_cpu_programs)

$(check_cpu_programs): check_cpu
	rm -f $@
	$(LN_S) -s check_cpu $@

install-exec-hook:
	cd $(DESTDIR)$(libexecdir) && \
	for i in $(check_cpu_programs) ; do rm -f $$i; ln -s check_cpu $$i ; done

clean-local:
	rm -f $(check_cpu_programs)

uninstall-local:
	cd $(DESTDIR)$(libexecdir) && rm -f $(check_cpu_programs)
