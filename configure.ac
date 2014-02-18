dnl Process this file with autoconf to produce a configure script.

m4_define(PROG_VERSION, 1)
m4_define(PROG_NAME, [nagios-plugins-linux])
m4_define(PROG_BUGREPORT, [davide.madrisan@gmail.com])

AC_INIT([PROG_NAME],[PROG_VERSION],[PROG_BUGREPORT],[PROG_NAME])
AC_PREREQ([2.59])

AC_CONFIG_SRCDIR([src/check_memory.c])
AC_CONFIG_HEADERS(src/config.h:src/config.hin)
AC_LANG([C])

AM_INIT_AUTOMAKE([gnu dist-bzip2])

AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for programs
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL

AC_PROG_RANLIB

dnl Check whether the compiler supports the __attribute__((weak, alias)) feature
#ac_save_CFLAGS="$CFLAGS"
#CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [if compiler supports __attribute__((weak, alias))],
  [cc_cv_attribute_alias],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void __f() { /* do something */; }
      void f() __attribute__((weak, alias("__f")));]])],
    [cc_cv_attribute_alias=yes],
    [cc_cv_attribute_alias=no])
  ])
#CFLAGS="$ac_save_CFLAGS"
if test "x$cc_cv_attribute_alias" = "xyes"; then
  AC_DEFINE(
    [SUPPORT_ATTRIBUTE_ALIAS], 1,
    [Define this if the compiler supports the alias attribute])
fi

dnl Check whether the compiler supports the __attribute__((__noreturn__)) feature
ac_cc_attribute_noreturn=
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_CACHE_CHECK(
  [if compiler supports __attribute__((__noreturn__))],
  [cc_cv_attribute_noreturn],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]],[[
      void f(void *foo) __attribute__((__noreturn__));]])],
    [cc_cv_attribute_noreturn=yes],
    [cc_cv_attribute_noreturn=no])
  ])
CFLAGS="$ac_save_CFLAGS"
if test "x$cc_cv_attribute_noreturn" = "xyes"; then
  ac_cc_attribute_noreturn='__attribute__((__noreturn__))'
fi
AC_DEFINE_UNQUOTED(
  [attribute_noreturn], [$ac_cc_attribute_noreturn],
  [Define this if the compiler supports the noreturn attribute])

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
  [attribute_format_printf(X,Y)], [$ac_cc_attribute_format_printf],
  [Define this if the compiler supports the format printf attribute])

dnl Checks for header files
AC_HEADER_STDC

AC_CHECK_HEADERS([ \
  sys/mount.h \
  sys/param.h \
  sys/sysctl.h \
  sys/sysinfo.h \
  sys/swap.h])

AC_CHECK_FUNCS([ \
  asprintf])

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

AC_PREFIX_DEFAULT(/usr/local/nagios)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_PROTOTYPES

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
cc_TRY_CFLAGS([-Wunreachable-code])
cc_TRY_CFLAGS([-Wshadow])
cc_TRY_CFLAGS([-Wsign-compare])
cc_TRY_CFLAGS([-Wunused])

AC_SUBST([CFLAGS], ["$SUPPORTED_CFLAGS $CFLAGS"])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
