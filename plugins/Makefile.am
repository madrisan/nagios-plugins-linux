AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include \
	-DMULTIPATHD_SOCKET=\"$(MULTIPATHD_SOCKET)\"

check_cpu_programs = check_iowait

libexec_PROGRAMS =        \
	check_clock       \
	check_cpu         \
	check_ifmountfs   \
	check_memory      \
	check_multipath   \
	check_nbprocs     \
	check_network     \
	check_paging      \
	check_readonlyfs  \
	check_swap        \
	check_temperature \
	check_tcpcount    \
	check_uptime      \
	check_users
if HAVE_GETLOADAVG
libexec_PROGRAMS += check_load
endif

check_clock_SOURCES      = check_clock.c
check_cpu_SOURCES        = check_cpu.c
check_ifmountfs_SOURCES  = check_ifmountfs.c
if HAVE_GETLOADAVG
check_load_SOURCES       = check_load.c
endif
check_memory_SOURCES     = check_memory.c
check_multipath_SOURCES  = check_multipath.c
check_nbprocs_SOURCES    = check_nbprocs.c
check_network_SOURCES    = check_network.c
check_paging_SOURCES     = check_paging.c
check_readonlyfs_SOURCES = check_readonlyfs.c
check_swap_SOURCES       = check_swap.c
check_tcpcount_SOURCES   = check_tcpcount.c
check_temperature_SOURCES = check_temperature.c
check_uptime_SOURCES     = check_uptime.c
check_users_SOURCES      = check_users.c

LDADD = $(top_srcdir)/lib/libutils.a

check_clock_LDADD        = $(LDADD)
check_cpu_LDADD          = $(LDADD)
check_ifmountfs_LDADD    = $(LDADD)
if HAVE_GETLOADAVG
check_load_LDADD         = $(LDADD)
endif
check_memory_LDADD       = $(LDADD)
check_nbprocs_LDADD      = $(LDADD)
check_network_LDADD      = $(LDADD)
check_multipath_LDADD    = $(LDADD)
check_paging_LDADD       = $(LDADD)
check_readonlyfs_LDADD   = $(LDADD)
check_swap_LDADD         = $(LDADD)
check_tcpcount_LDADD     = $(LDADD)
check_temperature_LDADD  = $(LDADD)
check_uptime_LDADD       = $(LDADD)
check_users_LDADD        = $(LDADD)

all-local: $(check_cpu_programs)

$(check_cpu_programs): check_cpu
	rm -f $@
	ln -s check_cpu $@

install-exec-hook:
	cd $(DESTDIR)$(libexecdir) && \
	for i in $(check_cpu_programs) ; do rm -f $$i; ln -s check_cpu $$i ; done

clean-local:
	rm -f $(check_cpu_programs)

uninstall-local:
	cd $(DESTDIR)$(libexecdir) && rm -f $(check_cpu_programs)
