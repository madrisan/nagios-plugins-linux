AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/include

test_programs = tsclock tscswch

LDADDS = $(top_builddir)/lib/libutils.a

tsclock_SOURCES = testutils.c testutils.h tsclock.c
tsclock_LDADD = $(LDADDS)

tscswch_SOURCES = testutils.c testutils.h tscswch.c
tscswch_LDADD = $(LDADDS)

noinst_PROGRAMS = $(test_programs)
TESTS = $(test_programs)
