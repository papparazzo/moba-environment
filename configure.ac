#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([moba-environment], [0.1.0-0000], [pappi-@gmx.de])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([wiringPi], [main])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_CHECK_PROG([MOBA_SHUTDOWN],[moba-shutdown],[yes],[no])
AM_CONDITIONAL([FOUND_MOBA_SHUTDOWN], [test "x$MOBA_SHUTDOWN" = xyes])
AM_COND_IF([FOUND_MOBA_SHUTDOWN],,[AC_MSG_ERROR([required program 'moba-shutdown' not found...])])

AC_CHECK_PROG([MOBA_AMBILIGHT],[moba-ambilight],[yes],[no])
AM_CONDITIONAL([FOUND_MOBA_AMBILIGHT], [test "x$MOBA_AMBILIGHT" = xyes])
AM_COND_IF([FOUND_MOBA_AMBILIGHT],,[AC_MSG_ERROR([required program 'moba-ambilight' not found...])])
