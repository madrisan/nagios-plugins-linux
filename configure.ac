dnl Process this file with autoconf to produce a configure script.

m4_define(PROG_VERSION, 12)
m4_define(PROG_NAME, [nagios-plugins-linux])
m4_define(PROG_BUGREPORT, [davide.madrisan@gmail.com])

AC_PREREQ([2.59])

AC_INIT([PROG_NAME],
        [PROG_VERSION],
        [PROG_BUGREPORT],
        [PROG_NAME])

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
        subdir-objects])

AC_CONFIG_SRCDIR([plugins/check_cpu.c])
AC_CONFIG_HEADERS(config.h:config.hin)
AC_CONFIG_LIBOBJ_DIR([lib])

AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Checks for programs
#AC_USE_SYSTEM_EXTENSIONS(_GNU_SOURCE)

AC_PROG_CC_STDC
AC_C_BIGENDIAN(
   [AC_DEFINE(WORDS_BIGENDIAN,1, [Define if byte ordering is bigendian])])

if test "$cross_compiling" = no; then
  if test "x$ac_cv_prog_cc_c99" = "xno" || test "x$ac_cv_prog_cc_c99" = "x"; then
  # We might be on RHEL5 with a git checkout and so broken
  # autoconf. Check if CC is gcc and if it bails when given -std=gnu99.
  # If not, use that.  Yuck.
    if test "x$ac_cv_c_compiler_gnu" = "xyes"; then
       CC="$CC -std=gnu99"
       AC_RUN_IFELSE(
          [AC_LANG_PROGRAM([],[[
  return 0;
      ]])],
      [],
      [AC_MSG_ERROR([Could not find a C99 compatible compiler])])
    else
      AC_MSG_ERROR([Could not find a C99 compatible compiler])
    fi
  fi
fi
AC_PROG_RANLIB

AM_PROG_CC_C_O

ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [if compiler supports __attribute__((__alloc_size__ args))],
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

ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [if compiler supports __attribute__((__malloc__))],
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

dnl Check whether the compiler supports the __attribute__((__noreturn__)) feature
ac_cc_attribute_noreturn=
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [if compiler supports the noreturn feature],
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
  [if compiler supports __attribute__((__format__ (__printf__, x,y))],
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
  linux/if_link.h \
  fcntl.h \
  limits.h \
  mntent.h \
  netinet/in.h \
  sys/mount.h \
  sys/param.h \
  sys/socket.h \
  sys/sysctl.h \
  sys/sysinfo.h \
  sys/time.h \
  sys/swap.h \
  utmpx.h])

AC_CHECK_FUNCS([asprintf])
AC_CHECK_FUNCS([getmntent])  dnl wanted by: lib/mountlist.c
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

dnl Check whether we can get the number of CPU
AC_CHECK_FUNCS([get_nprocs_conf get_nprocs], [],
  [AC_MSG_CHECKING([for number of online cpus])
   AC_TRY_COMPILE(
     [#include <unistd.h>],
     [sysconf(_SC_NPROCESSORS_ONLN) > 0;],
     AC_DEFINE(HAVE_SYSCONF__SC_NPROCESSORS_ONLN, 1,
               [Define if sysconf returns number of online cpus])
     AC_MSG_RESULT([sysconf(_SC_NPROCESSORS_ONLN)]),
     AC_MSG_RESULT([not available]))
   AC_MSG_CHECKING([for number of available cpus])
   AC_TRY_COMPILE(
     [#include <unistd.h>],
     [sysconf(_SC_NPROCESSORS_CONF) > 0;],
     AC_DEFINE(HAVE_SYSCONF__SC_NPROCESSORS_CONF, 1,
               [Define if sysconf returns number of available cpus])
     AC_MSG_RESULT([sysconf(_SC_NPROCESSORS_CONF)]),
     AC_MSG_RESULT([not available]))
  ])

have_func_getloadavg=false
AC_CHECK_FUNC([getloadavg], [have_func_getloadavg=true])
AM_CONDITIONAL([HAVE_GETLOADAVG], [test x$have_func_getloadavg = xtrue])

AC_CHECK_TYPE(struct sockaddr_in6,
  [AC_DEFINE(HAVE_AFINET6, 1,
    [Define to 1 if struct sockaddr_in6 is available])],
  [working_ipv6=no],
  [#include <sys/types.h>
   #include <netinet/in.h>])

AC_PREFIX_DEFAULT(/usr/local/nagios)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_PROTOTYPES
AC_C_INLINE

dnl cc_TRY_CFLAGS (CFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl ------------------------------------------------------------
dnl Checks if $CC supports a given set of CFLAGS.
dnl If supported, the current CFLAGS is appended to SUPPORTED_CFLAGS
AC_DEFUN([cc_TRY_CFLAGS],
   [AC_MSG_CHECKING([whether compiler accepts $1])
   ac_save_CFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS $1"
   AC_COMPILE_IFELSE([AC_LANG_SOURCE([
      int x;])],
      [ac_cv_try_cflags_ok=yes
       SUPPORTED_CFLAGS="$SUPPORTED_CFLAGS $1"],
      [ac_cv_try_cflags_ok=no])
   CFLAGS="$ac_save_CFLAGS"
   AC_MSG_RESULT([$ac_cv_try_cflags_ok])
   if test x"$ac_cv_try_cflags_ok" = x"yes"; then
      ifelse([$2],[],[:],[$2])
   else
      ifelse([$3],[],[:],[$3])
   fi
])  dnl end of cc_TRY_CFLAGS

cc_TRY_CFLAGS([-Wall])
cc_TRY_CFLAGS([-Wformat])
cc_TRY_CFLAGS([-Wformat-security])
cc_TRY_CFLAGS([-Wmissing-noreturn])
cc_TRY_CFLAGS([-Wmissing-format-attribute])
cc_TRY_CFLAGS([-Wshadow])
cc_TRY_CFLAGS([-Wsign-compare])
cc_TRY_CFLAGS([-Wunused])

AC_SUBST([CFLAGS], ["$SUPPORTED_CFLAGS $CFLAGS"])

dnl check for some Linux types
AC_TYPE_SIZE_T
# 'AC_TYPE_SSIZE_T' unfortunately requires autoconf 2.60+
# Define to `int' if <sys/types.h> does not define
AC_TYPE_SSIZE_T
AC_CHECK_TYPE(ssize_t, [int])
AC_TYPE_UID_T

dnl check the support for soft links
AC_PROG_LN_S

dnl extra option: '--enable-debug'
AC_ARG_ENABLE([debug],
   AS_HELP_STRING([--enable-debug],
      [enable debug messages @<:@default=disabled@:>@]),
      [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
   AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])])

dnl extra option: '--with-socketfile'
MULTIPATHD_SOCKET="/var/run/multipathd.sock"
AC_ARG_WITH(
  [socketfile],
  [AS_HELP_STRING(
    [--with-socketfile],
    [use a different socket file
     (default is /var/run/multipathd.sock)])],
  [MULTIPATHD_SOCKET="$with_socketfile"])
AC_SUBST(MULTIPATHD_SOCKET)

AC_CONFIG_FILES([\
  Makefile \
  include/Makefile \
  lib/Makefile \
  plugins/Makefile])

AC_OUTPUT
