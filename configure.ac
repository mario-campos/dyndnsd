AC_PREREQ([2.69])
AC_INIT([dyndnsd], [0.0.0], [mario@mariocampos.io])
AC_CONFIG_SRCDIR([src/dyndnsd.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 subdir-objects foreign -Wall])

AC_ARG_WITH([user],
	    AS_HELP_STRING([--with-user],
			   [Set the default process user @<:@nobody@:>@]),
	    [with_user="$withval"],
	    [with_user="nobody"])
AC_ARG_WITH([group],
	    AS_HELP_STRING([--with-group],
			   [Set the default process group @<:@nobody@:>@]),
	    [with_group="$withval"],
	    [with_group="nobody"])
AC_ARG_WITH([conf-path],
	    AS_HELP_STRING([--with-conf-path],
			   [Set the default configuration file path @<:@/etc/dyndnsd.conf@:>@]),
	    [with_conf_path="$withval"],
	    [with_conf_path="/etc/dyndnsd.conf"])

AC_DEFINE_UNQUOTED([DYNDNSD_USER],
		   ["$with_user"],
		   [default process user])
AC_DEFINE_UNQUOTED([DYNDNSD_GROUP],
		   ["$with_group"],
		   [default process group])
AC_DEFINE_UNQUOTED([DYNDNSD_CONF_PATH],
		   ["$with_conf_path"],
		   [default configuration file path])

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
AC_CHECK_FUNCS([memset pledge daemon getifaddrs explicit_bzero])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
