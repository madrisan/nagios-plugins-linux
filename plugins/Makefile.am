AM_CPPFLAGS = \
   -include $(top_builddir)/config.h \
   -I$(top_srcdir)/lib
AM_CFLAGS = -DMULTIPATHD_SOCKET=\"$(MULTIPATHD_SOCKET)\"

check_cpu_programs = check_iowait

libexec_PROGRAMS = \
   check_cpu \
   check_ifmountfs \
   check_memory \
   check_multipath \
   check_readonlyfs \
   check_swap \
   check_uptime \
   check_users
if HAVE_GETLOADAVG
libexec_PROGRAMS += check_load
endif

check_cpu_SOURCES = check_cpu.c
check_ifmountfs_SOURCES = check_ifmountfs.c
if HAVE_GETLOADAVG
check_load_SOURCES = check_load.c
endif
check_memory_SOURCES = check_memory.c
check_multipath_SOURCES = check_multipath.c
check_readonlyfs_SOURCES = check_readonlyfs.c
check_swap_SOURCES = check_swap.c
check_uptime_SOURCES = check_uptime.c
check_users_SOURCES = check_users.c

LDADD = ../lib/libnagiosplugins.a

check_cpu_LDADD = $(LDADD)
check_ifmountfs_LDADD = $(LDADD)
if HAVE_GETLOADAVG
check_load_LDADD = $(LDADD)
endif
check_memory_LDADD = $(LDADD)
check_multipath_LDADD = $(LDADD)
check_readonlyfs_LDADD = $(LDADD)
check_swap_LDADD = $(LDADD)
check_uptime_LDADD = $(LDADD)
check_users_LDADD = $(LDADD)

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