AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/tests

test_programs = tsclock

LDADDS = $(top_builddir)/lib/libutils.a

tsclock_SOURCES = testutils.c testutils.h tsclock.c
tsclock_LDADD = $(LDADDS)

noinst_PROGRAMS = $(test_programs)
TESTS = $(test_programs)
