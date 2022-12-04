#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([moba-environment], [0.2.0-0000], [pappi-@gmx.de])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SUBDIRS([modules/lib-msghandling])

AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_MACRO_DIR([m4])


PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.1])

AC_CHECK_LIB([mobacommon], [wiringPi], [main])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

AX_CXX_COMPILE_STDCXX_17()

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

AC_OUTPUT

AC_CHECK_PROG([MOBA_SHUTDOWN],[moba-shutdown],[yes],[no])
AM_CONDITIONAL([FOUND_MOBA_SHUTDOWN], [test "x$MOBA_SHUTDOWN" = xyes])
AM_COND_IF([FOUND_MOBA_SHUTDOWN],,[AC_MSG_ERROR([required program 'moba-shutdown' not found...])])

AC_CHECK_PROG([MOBA_AMBILIGHT],[moba-ambilight],[yes],[no])
AM_CONDITIONAL([FOUND_MOBA_AMBILIGHT], [test "x$MOBA_AMBILIGHT" = xyes])
AM_COND_IF([FOUND_MOBA_AMBILIGHT],,[AC_MSG_ERROR([required program 'moba-ambilight' not found...])])
