AC_PREREQ([2.60])
AC_INIT([libcopyfile], [0])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 subdir-objects])

AC_PROG_CC([c99 cc])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([disable-static])

AC_CHECK_FUNCS([fchmodat lchown posix_fallocate utimensat])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [Enable stderr debug]))

AC_ARG_ENABLE([libattr],
	AS_HELP_STRING([--disable-libattr],
		[Disable use of libattr (default: autodetect)]))

AS_IF([test x"$enable_libattr" != x"no"],
[
	AC_CHECK_LIB([attr], [getxattr])
])

AS_IF([test x"$enable_debug" = x"yes"],
[
	AC_DEFINE([ENABLE_DEBUG], [1], [Enable debugging code])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
