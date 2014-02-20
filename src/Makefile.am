AM_CPPFLAGS = -I$(top_srcdir)/lib

libexec_PROGRAMS = \
   check_ifmountfs \
   check_memory \
   check_readonlyfs \
   check_swap \
   check_uptime

check_memory_SOURCES = check_memory.c
check_swap_SOURCES = check_swap.c
check_uptime_SOURCES = check_uptime.c
check_readonlyfs_SOURCES = check_readonlyfs.c
check_ifmountfs_SOURCES = check_ifmountfs.c

LDADD = ../lib/libnagiosplugins.a

check_memory_LDADD = $(LDADD)
check_swap_LDADD = $(LDADD)
check_uptime_LDADD = $(LDADD)
check_readonlyfs_LDADD = $(LDADD)
check_ifmountfs_LDADD = $(LDADD)
