AC_PREREQ([2.69])
AC_INIT([dyndnsd], [0.0.0], [mario@mariocampos.io])
AC_CONFIG_SRCDIR([src/dyndnsd.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 subdir-objects foreign -Wall])
AM_SILENT_RULES([yes])

AC_DEFINE([_PATH_DYNDNSD_CONF], ["/etc/dyndnsd.conf"], [Default configuration file])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_YACC
AM_PROG_LEX
AC_PROG_INSTALL

AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h sys/socket.h syslog.h unistd.h])
AC_CHECK_HEADER_STDBOOL

AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset pledge daemon getifaddrs])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
