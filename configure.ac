dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT([nagios-plugins-linux],[19],
        [davide.madrisan@gmail.com])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([
        check-news
        gnu
        1.11
        silent-rules
        tar-pax
        no-dist-gzip
        dist-bzip2
        dist-xz
        subdir-objects
        -Wno-portability
])

AC_CONFIG_SRCDIR([plugins/check_cpu.c])
AC_CONFIG_HEADERS(config.h:config.hin)
AC_CONFIG_LIBOBJ_DIR([lib])
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

#AC_USE_SYSTEM_EXTENSIONS(_GNU_SOURCE)

dnl Checks for the compiler features

AC_C_CONST
AC_C_PROTOTYPES
AC_C_INLINE

m4_ifndef([LT_INIT], [
  AM_PROG_LIBTOOL
], [
  LT_INIT([shared disable-static])
])

# 'AC_PROG_CC_C99' unfortunately requires autoconf 2.60+
#AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CC_STDC

if test "$cross_compiling" = no; then
  if test "x$ac_cv_prog_cc_c99" = "xno" || test "x$ac_cv_prog_cc_c99" = "x"; then
    # Check if CC is gcc and if it bails when given -std=gnu99.
    # If not, use that.  Yuck.
    if test "x$ac_cv_c_compiler_gnu" = "xyes"; then
      CC="$CC -std=gnu99"
      AC_RUN_IFELSE(
        [AC_LANG_PROGRAM([],[[return 0;]])],
        [],
        [AC_MSG_ERROR([Could not find a C99 compatible compiler])],
        [AC_MSG_ERROR([Internal error: not reached in cross-compile])])
    else
      AC_MSG_ERROR([Could not find a C99 compatible compiler])
    fi
  fi
fi

dnl Check for variable-length arrays support
AC_MSG_CHECKING(for variable-lenght arrays)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[]],[[
    int foo;
    foo = 10;
    int array[foo];]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([VARIABLE_LENGTH_ARRAYS], [],
             [Define this if the compiler supports variable-length arrays])],
  [AC_MSG_RESULT([no])
   AC_MSG_ERROR([The compiler does not support variable-length arrays])
  ])

dnl Checks for some compiler warning flags

cc_TRY_CFLAGS([-Wall])
cc_TRY_CFLAGS([-Wformat])
cc_TRY_CFLAGS([-Wformat-security])
cc_TRY_CFLAGS([-Wmissing-noreturn])
cc_TRY_CFLAGS([-Wmissing-format-attribute])
cc_TRY_CFLAGS([-Wredundant-decls])
cc_TRY_CFLAGS([-Wshadow])
cc_TRY_CFLAGS([-Wsign-compare])
cc_TRY_CFLAGS([-Wstrict-aliasing=2])
cc_TRY_CFLAGS([-Wunused])
# The gcc compiler shipped by RHEL5 (gcc-4.1.2) does not recognize it
#cc_TRY_CFLAGS([-Warray-bounds])

AC_SUBST([CFLAGS], ["$SUPPORTED_CFLAGS $CFLAGS"])

dnl Checks whether the compiler supports the
dnl    __attribute__((__alloc_size__ args)) feature
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [whether compiler supports __attribute__((__alloc_size__ args))],
  [cc_cv_attribute_alloc_size],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void *fooalloc(int n, int s) __attribute__((__alloc_size__(1, 2)));]])],
    [cc_cv_attribute_alloc_size=yes],
    [cc_cv_attribute_alloc_size=no])
  ])
CFLAGS="$ac_save_CFLAGS"
if test "x$cc_cv_attribute_alloc_size" = "xyes"; then
  ac_cc_attribute_alloc_size='__attribute__((__alloc_size__ args))'
fi
AC_DEFINE_UNQUOTED(
  [_attribute_alloc_size_(args)], [$ac_cc_attribute_alloc_size],
  [Define this if the compiler supports the alloc_size attribute])

dnl Checks whether the compiler supports the
dnl    __attribute__((__malloc__)) feature
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [whether compiler supports __attribute__((__malloc__))],
  [cc_cv_attribute_malloc],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void *fooalloc(int size) __attribute__((__malloc__));]])],
    [cc_cv_attribute_malloc=yes],
    [cc_cv_attribute_malloc=no])
  ])
CFLAGS="$ac_save_CFLAGS"
if test "x$cc_cv_attribute_malloc" = "xyes"; then
  ac_cc_attribute_malloc='__attribute__((__malloc__))'
fi
AC_DEFINE_UNQUOTED(
  [_attribute_malloc_], [$ac_cc_attribute_malloc],
  [Define this if the compiler supports the malloc attribute])

dnl Check whether the compiler supports the
dnl    __attribute__((__noreturn__)) feature
ac_cc_attribute_noreturn=
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [whether compiler supports the noreturn feature],
  [cc_cv_attribute_noreturn],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void _Noreturn f(void *foo);]])],
    [cc_cv_attribute_noreturn="_Noreturn"],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[]],[[
        void f(void *foo) __attribute__((__noreturn__));]])],
      [cc_cv_attribute_noreturn="using __attribute__"],
      [cc_cv_attribute_noreturn=no])
    ])
  ])
CFLAGS="$ac_save_CFLAGS"
case "$cc_cv_attribute_noreturn" in
  "using __attribute__")
    AC_DEFINE_UNQUOTED(
      [_Noreturn], [__attribute__((__noreturn__))],
      [Define this if the compiler supports the noreturn __attribute__])
  ;;
  "no")
    AC_DEFINE_UNQUOTED([_Noreturn], [],
      [Define this if the compiler does not supports any noreturn attribute])
  ;;
esac

dnl Check whether the compiler supports the 
dnl    __attribute__((__format__ (__printf__, x,y))) feature
ac_cc_attribute_format_printf=
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [whether compiler supports __attribute__((__format__ (__printf__, x,y))],
  [cc_cv_attribute_format_printf],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void f(const char *fmt, ...)
         __attribute__((__format__ (__printf__, 1,2)));]])],
    [cc_cv_attribute_format_printf=yes],
    [cc_cv_attribute_format_printf=no])
  ])
CFLAGS="$ac_save_CFLAGS"
if test "x$cc_cv_attribute_format_printf" = "xyes"; then
  ac_cc_attribute_format_printf='__attribute__((__format__ (__printf__, X,Y)))'
fi
AC_DEFINE_UNQUOTED(
  [_attribute_format_printf_(X,Y)], [$ac_cc_attribute_format_printf],
  [Define this if the compiler supports the format printf attribute])

dnl Checks for header files
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
  arpa/inet.h \
  fcntl.h \
  limits.h \
  mntent.h \
  netinet/in.h \
  sys/mount.h \
  sys/param.h \
  sys/socket.h \
  sys/sysctl.h \
  sys/sysinfo.h \
  sys/types.h \
  sys/time.h \
  sys/swap.h \
  utmpx.h])

AC_CHECK_HEADERS(linux/if_link.h, [], [],
[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

dnl Checks for functions and libraries

AC_CHECK_FUNCS([asprintf])

dnl Check for clock_gettime CLOCK_MONOTONIC
dnl wanted by: plugins/check_uptime.c
AC_CHECK_LIB([rt], [clock_gettime])
AC_MSG_CHECKING([for clock_gettime with clock CLOCK_MONOTONIC])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <time.h>]],
    [[struct timespec ts;
      clock_gettime (CLOCK_MONOTONIC, &ts);]])],
  [ac_cv_clock_gettime_monotonic=yes
   AC_DEFINE_UNQUOTED(HAVE_CLOCK_GETTIME_MONOTONIC, 1,
     [Define to 1 if you have clock_gettime(CLOCK_MONOTONIC).])
  ],
  [ac_cv_clock_gettime_monotonic=no])
AC_MSG_RESULT([$ac_cv_clock_gettime_monotonic])

dnl suggestions from autoscan
AC_CHECK_FUNCS([getmntent])  dnl wanted by: lib/mountlist.c
AC_CHECK_FUNCS([hasmntopt])  dnl wanted by: lib/mountlist.c
AC_CHECK_FUNCS([memset])     dnl wanted by: lib/cpustats.c
AC_CHECK_FUNCS([regcomp])    dnl wanted by: plugins/check_multipath.c
AC_CHECK_FUNCS([socket])     dnl wanted by: plugins/check_multipath.c
AC_CHECK_FUNCS([strchr])     dnl wanted by: lib/interrupts.c
AC_CHECK_FUNCS([strerror])   dnl wanted by: lib/thresholds.c
AC_CHECK_FUNCS([strrchr])    dnl wanted by: lib/progname.c
AC_CHECK_FUNCS([strstr])     dnl wanted by: lib/cpudesc.c
AC_CHECK_FUNCS([strtol])     dnl wanted by: lib/processes.c
AC_CHECK_FUNCS([strtoul])    dnl wanted by: lib/interrupts.c
AC_CHECK_FUNCS([strtoull])   dnl wanted by: lib/procparser.c
AC_CHECK_FUNCS([sysinfo])    dnl wanted by: plugins/check_uptime.c
AC_CHECK_FUNCS([uname])      dnl wanted by: lib/cpudesc.c
AC_CHECK_FUNCS([realpath])   dnl wanted by: plugins/check_fc.c
AC_CHECK_FUNCS([strtoull])   dnl wanted by: plugins/check_fc.c

AC_FUNC_GETMNTENT
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD

dnl sched.h: check for the macros CPU_*
AC_CHECK_TYPES([cpu_set_t], [have_cpu_set_t=yes], [], [[#include <sched.h>]])
AM_CONDITIONAL([HAVE_CPU_SET_T], [test "x$have_cpu_set_t" = xyes])
AC_CHECK_DECLS([CPU_ALLOC], [], [],
  [[#define _GNU_SOURCE
    #include <sched.h>]])

dnl Add the option '--with-proc-meminfo=PATH'
AC_ARG_WITH(proc-meminfo,
  AS_HELP_STRING([--with-proc-meminfo=PATH],
    [path to /proc/meminfo or equivalent]),
  []
  [with_procmeminfo=$withval])
dnl dunno why this does not work below - use hack (kbd)
dnl fine on linux, broken on solaris
dnl if /bin/test -e "/proc/meminfo"
AC_MSG_CHECKING([for /proc/meminfo])
if test -n "$with_procmeminfo"; then
  AC_MSG_RESULT([(command line) $with_procmeminfo])
elif [cat /proc/meminfo > /dev/null 2>&1]; then
  AC_MSG_RESULT([found /proc/meminfo])
  with_procmeminfo="/proc/meminfo"
else
  AC_MSG_RESULT([no])
fi
if test -n "$with_procmeminfo"; then
  AC_DEFINE(HAVE_PROC_MEMINFO,1,[Define if we have /proc/meminfo])
  AC_DEFINE_UNQUOTED(PROC_MEMINFO,"$with_procmeminfo",[path to /proc/meminfo if name changes])
fi

if test -z "$with_procmeminfo"; then
  AC_MSG_FAILURE([no /proc/meminfo (or equivalent) found])
fi

dnl Add the option: '--enable-debug'
AC_ARG_ENABLE([debug],
   AS_HELP_STRING([--enable-debug],
      [enable debug messages @<:@default=disabled@:>@]),
      [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
   AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])])

dnl Add the option: '--with-socketfile'
MULTIPATHD_SOCKET="@/org/kernel/linux/storage/multipathd"
AC_ARG_WITH(
  [socketfile],
  [AS_HELP_STRING(
    [--with-socketfile],
    [use a different socket file
     (default is @/org/kernel/linux/storage/multipathd)])],
  [MULTIPATHD_SOCKET="$with_socketfile"])
AC_SUBST(MULTIPATHD_SOCKET)

dnl Add the option: '--with-tests'
AC_ARG_WITH(
  [test-suite],
  [AS_HELP_STRING(
    [--with-test-suite],
    [build test suite by default @<:@default=check@:>@])],
  [case "${withval}" in
    yes|no|check) ;;
    *) AC_MSG_ERROR([bad value ${withval} for tests option]) ;;
   esac],
  [withval=check])
AC_MSG_CHECKING([Whether to build test suite by default])
if test "$withval" = "check" ; then
  if test -d $srcdir/.git ; then
    withval=yes
  else
    withval=no
  fi
fi
AC_MSG_RESULT([$withval])
AM_CONDITIONAL([WITH_TESTS], [test "$withval" = "yes"])

dnl Check whether we can get the number of CPU
AC_CHECK_FUNCS([get_nprocs_conf get_nprocs], [],
  [AC_MSG_CHECKING([for number of online cpus])
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[#include <unistd.h>]],[[
       sysconf(_SC_NPROCESSORS_ONLN) > 0;]])],
     [AC_MSG_RESULT([sysconf(_SC_NPROCESSORS_ONLN)])
      AC_DEFINE(HAVE_SYSCONF__SC_NPROCESSORS_ONLN, 1,
                [Define if sysconf returns number of online cpus])],
     [AC_MSG_RESULT([not available])])
   AC_MSG_CHECKING([for number of available cpus])
   AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[#include <unistd.h>]],[[
       sysconf(_SC_NPROCESSORS_CONF) > 0;]])],
     [AC_MSG_RESULT([sysconf(_SC_NPROCESSORS_CONF)])
      AC_DEFINE(HAVE_SYSCONF__SC_NPROCESSORS_CONF, 1,
                [Define if sysconf returns number of available cpus])],
     [AC_MSG_RESULT([not available])]
   )])

dnl Checks for the function getloadavg
have_func_getloadavg=false
AC_CHECK_FUNC([getloadavg], [have_func_getloadavg=true])
AM_CONDITIONAL([HAVE_GETLOADAVG], [test x$have_func_getloadavg = xtrue])

dnl Checks for struct sockaddr_in6
AC_CHECK_TYPES([struct sockaddr_in6],
  [AC_DEFINE(HAVE_AFINET6, 1,
    [Define to 1 if struct sockaddr_in6 is available])],
  [working_ipv6=no],
  [#include <sys/types.h>
   #include <netinet/in.h>])

dnl Check for some Linux types
AC_TYPE_SIZE_T
# 'AC_TYPE_SSIZE_T' unfortunately requires autoconf 2.60+
#AC_TYPE_SSIZE_T
# Define to `int' if <sys/types.h> does not define
AC_CHECK_TYPES([ssize_t])
AC_TYPE_UID_T
AC_TYPE_UINT64_T

dnl Checks for programs
AC_PROG_RANLIB

dnl Check the support for soft links
AC_PROG_LN_S

dnl Set the default --prefix
AC_PREFIX_DEFAULT(/usr/local/nagios)

AC_CONFIG_FILES([\
  Makefile \
  include/Makefile \
  lib/Makefile \
  plugins/Makefile \
  tests/Makefile])

AC_OUTPUT
