AM_CPPFLAGS = -I$(top_srcdir)/lib

libexec_PROGRAMS = \
   check_ifmountfs \
   check_io_latency \
   check_memory \
   check_readonlyfs \
   check_swap \
   check_uptime
if HAVE_GETLOADAVG
libexec_PROGRAMS += check_load
endif

check_ifmountfs_SOURCES = check_ifmountfs.c
check_io_latency_SOURCES = check_io_latency.c
if HAVE_GETLOADAVG
check_load_SOURCES = check_load.c
endif
check_memory_SOURCES = check_memory.c
check_readonlyfs_SOURCES = check_readonlyfs.c
check_swap_SOURCES = check_swap.c
check_uptime_SOURCES = check_uptime.c

LDADD = ../lib/libnagiosplugins.a

check_ifmountfs_LDADD = $(LDADD)
check_io_latency_LDADD = $(LDADD)
if HAVE_GETLOADAVG
check_load_LDADD = $(LDADD)
endif
check_memory_LDADD = $(LDADD)
check_readonlyfs_LDADD = $(LDADD)
check_swap_LDADD = $(LDADD)
check_uptime_LDADD = $(LDADD)

