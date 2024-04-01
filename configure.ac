dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT(nagios-plugins-linux,
        m4_normalize(m4_include([VERSION])),
        [davide.madrisan@gmail.com])

AC_MSG_CHECKING([for $PACKAGE_NAME version])
AC_MSG_RESULT([$PACKAGE_VERSION])

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
AC_PROG_CC

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

# Linux Alpine shipis with musl libc
AC_MSG_CHECKING(for musl libc)
dumpmachine=`$CC -dumpmachine`
case "$dumpmachine" in
*-musl)
  AC_MSG_RESULT([yes])
  AC_DEFINE([LIBC_MUSL], [1], [Define to 1 if libc is musl])
  ;;
*)
  AC_MSG_RESULT([no]) ;;
esac

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

dnl Add the option '--disable-hardening'
AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--disable-hardening],
    [do not attempt to harden the resulting executables (default is to harden)])],
    [use_hardening=$enableval],
    [use_hardening=yes])

dnl Turn warnings into errors: '--enable-werror'
AC_ARG_ENABLE([werror],
  AS_HELP_STRING([--enable-werror],
    [make all warnings as errors (default is no)]),
    [enable_werror=$enableval],
    [enable_werror=no])
AS_IF([test "x$enable_werror" = "xyes"], [
  cc_TRY_CFLAGS([-Werror])
  case "$SUPPORTED_CFLAGS" in
    *-Werror*) ;;
    *)
      AC_MSG_ERROR([enable-werror set but -Werror is not usable])
      ;;
  esac])

dnl Checks for some compiler warning flags

cc_TRY_CFLAGS([-Wall])
cc_TRY_CFLAGS([-Wformat])
cc_TRY_CFLAGS([-Wformat -Wformat-security])
cc_TRY_CFLAGS([-Wformat-signedness])
cc_TRY_CFLAGS([-Wimplicit-fallthrough=2])
cc_TRY_CFLAGS([-Wmissing-noreturn])
cc_TRY_CFLAGS([-Wmissing-format-attribute])
cc_TRY_CFLAGS([-Wredundant-decls])
cc_TRY_CFLAGS([-Wshadow])
cc_TRY_CFLAGS([-Wsign-compare])
cc_TRY_CFLAGS([-Wstrict-aliasing=2])
cc_TRY_CFLAGS([-Wstringop-truncation])
cc_TRY_CFLAGS([-Wunused])
# The gcc compiler shipped by RHEL5 (gcc-4.1.2) does not recognize it
#cc_TRY_CFLAGS([-Warray-bounds])

if test x$use_hardening != xno; then
  cc_TRY_CFLAGS([-Wstack-protector])
  cc_TRY_CFLAGS([-D_FORTIFY_SOURCE=2])
  cc_TRY_CFLAGS([-fstack-protector-strong])
  cc_TRY_CFLAGS([-fpie -fPIE])
  case "$SUPPORTED_CFLAGS" in
    *-fstack-protector-strong*) ;;
    *)
      cc_TRY_CFLAGS([-fstack-protector-all])
      ;;
  esac
  cc_TRY_LDFLAGS([-Wl,-z,relro])
  cc_TRY_LDFLAGS([-Wl,-z,now])
fi

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
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
  arpa/inet.h \
  fcntl.h \
  limits.h \
  mntent.h \
  netinet/in.h \
  sys/mount.h \
  sys/param.h \
  sys/resource.h \
  sys/socket.h \
  sys/sysctl.h \
  sys/sysinfo.h \
  sys/types.h \
  sys/time.h \
  sys/swap.h \
  utmpx.h])

AC_CHECK_HEADERS([ \
  linux/if_link.h \
  linux/if.h \
  linux/wireless.h], [], [],
[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

dnl check for headers required by the netinfo* libraries
AC_CHECK_HEADERS([ \
  linux/ethtool.h \
  linux/netlink.h \
  linux/rtnetlink.h \
  linux/sockios.h], [],
  [AC_MSG_ERROR([please install linux network headers])])

dnl Checks for functions and libraries

AC_CHECK_FUNCS([asprintf])

AC_CHECK_FUNCS([ \
  __secure_getenv \
  secure_getenv\
])

dnl Check for ceil (some platforms use libm for the math functions
dnl instead of the C library
dnl wanted by lib/netinfo.c

LIBS_SAVE="$LIBS"
AC_SEARCH_LIBS([ceil], [m], [], [
  AC_MSG_ERROR([unable to find the ceil() function])
])
CEIL_LIBS="$LIBS"
AC_SUBST([CEIL_LIBS])
LIBS="$LIBS_SAVE"

dnl Check for clock_gettime CLOCK_MONOTONIC
dnl wanted by: plugins/check_uptime.c
LIBS_SAVE="$LIBS"
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
CLOCK_LIBS="$LIBS"
AC_SUBST([CLOCK_LIBS])
LIBS="$LIBS_SAVE"

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

dnl Check for libcurl
AC_ARG_ENABLE([libcurl],
  AS_HELP_STRING([--enable-libcurl], [enable libcurl]))
AS_IF([test "x$enable_libcurl" = "xyes"], [
  LIBCURL_CHECK_CONFIG([], [7.40.0], [],
    [AC_MSG_ERROR([Missing required libcurl >= 7.40.0])])
  AC_SUBST([LIBCURL_CPPFLAGS])
  AC_SUBST([LIBCURL])
  AM_CONDITIONAL(HAVE_LIBCURL, [test "$libcurl_cv_lib_curl_usable" = "yes"])
], [AM_CONDITIONAL(HAVE_LIBCURL, false)])

dnl Check for the procps newlib
have_libprocps=no
AC_ARG_ENABLE([libprocps],
  AS_HELP_STRING([--enable-libprocps], [enable optional LIBPROC2 support]),
    [enable_libprocps=$enableval],[enable_libprocps=no])
AS_IF([test "x$enable_libprocps" = "xyes"], [
  PKG_CHECK_EXISTS([libproc2],
    [PKG_CHECK_MODULES(LIBPROCPS, [libproc2 >= 4.0.0],
      [AC_DEFINE(HAVE_LIBPROCPS, 1, [Define if libproc2 (procps) is available])
       have_libprocps=yes],
      AC_MSG_ERROR([*** libproc2 (procps) version >= 4.0.0 not found]))])
  AC_SUBST(LIBPROCPS_LIBS)
  AC_SUBST(LIBPROCPS_CFLAGS)
  AS_IF([test "x$have_libprocps" = "xno" -a "x$enable_libprocps" = "xyes"],
        [AC_MSG_ERROR([*** libproc2 (procps) >= 4.0.0 support requested, but libraries not found])])
])
AM_CONDITIONAL(HAVE_LIBPROCPS, [test "$have_libprocps" = "yes"])

dnl Check for systemd-login libraries
AC_ARG_ENABLE([systemd],
  AS_HELP_STRING([--enable-systemd], [enable systemd related features]),
  [], [enable_systemd=check]
)
have_systemd=no
AS_IF([test "x$enable_systemd" != "xno"], [
  PKG_CHECK_MODULES([SYSTEMD], [libsystemd], [have_systemd=yes], [have_systemd=no])
  AS_CASE([$enable_systemd:$have_systemd],
    [yes:no],
      [AC_MSG_ERROR([systemd expected but libsystemd not found])],
    [*:yes],
       AC_DEFINE([HAVE_LIBSYSTEMD], [1], [Define if libsystemd is available])
       AC_DEFINE([USE_SYSTEMD], [1], [Define if systemd support is wanted])
  )
])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$have_systemd" = "xyes"])

dnl Add the option '--with-proc-meminfo=PATH'
AC_ARG_WITH(proc-meminfo,
  AS_HELP_STRING([--with-proc-meminfo=PATH],
    [path to /proc/meminfo or equivalent]),
  []
  [with_procmeminfo=$withval])
AC_MSG_CHECKING([for /proc/meminfo])
if test x${with_procmeminfo+set} = xset; then
  AC_MSG_RESULT([(command line) $with_procmeminfo])
elif [cat /proc/meminfo > /dev/null 2>&1]; then
  AC_MSG_RESULT([found /proc/meminfo])
  with_procmeminfo="/proc/meminfo"
else
  AC_MSG_RESULT([no])
fi
if test x${with_procmeminfo+set} = xset; then
  AC_DEFINE_UNQUOTED(PROC_MEMINFO,"$with_procmeminfo",[path to /proc/meminfo if name changes])
fi
AM_CONDITIONAL([HAVE_PROC_MEMINFO], [test x${with_procmeminfo+set} = xset])

dnl Add the option: '--enable-debug'
AC_ARG_ENABLE([debug],
   AS_HELP_STRING([--enable-debug],
      [enable debug messages @<:@default=disabled@:>@]),
      [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
   AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])])

dnl Add the option: '--with-docker-socket'
DOCKER_SOCKET="/var/run/docker.sock"
AC_ARG_WITH(
  [dockersocket],
  [AS_HELP_STRING(
    [--with-docker-socket],
    [use a different socket file for docker
     (default is /var/run/docker.sock)])],
  [DOCKER_SOCKET="$with_dockersocket"])
AC_SUBST(DOCKER_SOCKET)

dnl Add the option: '--with-podman-socket'
PODMAN_SOCKET="/run/podman/podman.sock"
AC_ARG_WITH(
  [podmansocket],
  [AS_HELP_STRING(
    [--with-podman-socket],
    [use a different podman socket
     (default is /run/podman/podman.sock)])],
  [PODMAN_SOCKET="$with_podmansocket"])
AC_SUBST(PODMAN_SOCKET)

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
  [case "${with_test_suite}" in
    yes|no|check) ;;
    *) AC_MSG_ERROR([bad value ${with_test_suite} for tests option]) ;;
   esac],
  [with_test_suite=check])
AC_MSG_CHECKING([Whether to build test suite by default])
if test "$with_test_suite" = "check" ; then
  if test -d $srcdir/.git ; then
    with_test_suite=yes
  else
    with_test_suite=no
  fi
fi
AC_MSG_RESULT([$with_test_suite])
AM_CONDITIONAL([WITH_TESTS], [test "$with_test_suite" = "yes"])

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
LT_INIT

dnl Check the support for soft links
AC_PROG_LN_S

dnl Set the default --prefix
AC_PREFIX_DEFAULT(/usr/local/nagios)

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

AC_SUBST([CFLAGS], ["$SUPPORTED_CFLAGS $CFLAGS"])
AC_SUBST([LDFLAGS], ["$SUPPORTED_LDFLAGS $LDFLAGS"])
AC_SUBST([LIBTOOL_DEPS])

AC_CONFIG_FILES([\
  Makefile \
  debian/Makefile \
  include/Makefile \
  lib/Makefile \
  packages/Makefile \
  packages/specs/Makefile \
  plugins/Makefile \
  tests/Makefile])

AC_OUTPUT

echo
echo "Options used to compile and link:"
echo "  debug enabled      = $enable_debug"
echo "  hardening enabled  = $use_hardening"
echo "  werror enabled     = $enable_werror"
echo "  with docker socket = $DOCKER_SOCKET"
echo "  with libprocps     = $enable_libprocps"
echo "  with podman socket = $PODMAN_SOCKET"
echo "  with socketfile    = $MULTIPATHD_SOCKET"
echo "  with test suite    = $with_test_suite"
echo
echo "  target os          = $target_os"
echo "  build os           = $build_os"
echo
echo "  CC                 = $CC"
echo "  CFLAGS             = $CFLAGS"
echo "  LDFLAGS            = $LDFLAGS"
echo

if test "$have_libprocps" = "yes"; then
  echo "Optional procps-ng library support is enabled:"
  echo "  LIBPROCPS_CFLAGS   = $LIBPROCPS_CFLAGS"
  echo "  LIBPROCPS_LIBS     = $LIBPROCPS_LIBS"
  echo
fi

if test "$libcurl_cv_lib_curl_usable" = "yes"; then
  echo "Optional curl library support is enabled (required by check_container):"
  echo "  LIBCURL_CPPFLAGS   = $LIBCURL_CPPFLAGS"
  echo "  LIBCURL            = $LIBCURL"
  echo
fi

if test "$have_systemd" = "yes"; then
  echo "Optional systemd library support is enabled:"
  echo "  SYSTEMD_CFLAGS    = $SYSTEMD_CFLAGS"
  echo "  SYSTEMD_LIBS      = $SYSTEMD_LIBS"
fi
