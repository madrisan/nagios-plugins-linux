libexec_PROGRAMS = \
   check_memory \
   check_swap \
   check_uptime

noinst_LIBRARIES = libnp.a
libnp_a_SOURCES = nputils.c nputils.h

check_memory_SOURCES = \
	check_memory.c \
	meminfo.h meminfo.c
check_memory_LDADD = libnp.a

check_swap_SOURCES = \
	check_swap.c \
	meminfo.h meminfo.c
check_swap_LDADD = libnp.a

check_uptime_SOURCES = check_uptime.c
check_uptime_LDADD = libnp.a

