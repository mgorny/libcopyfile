AC_PREREQ([2.60])
AC_INIT([libcopyfile], [0])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 subdir-objects])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([disable-static])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [Enable stderr debug]))

AC_ARG_ENABLE([libacl],
	AS_HELP_STRING([--disable-libacl],
		[Disable use of libacl (default: autodetect)]))

AS_IF([test x"$enable_libacl" != x"no"],
[
	AC_SEARCH_LIBS([acl_get_file], [acl],
	[
		AC_DEFINE([HAVE_LIBACL], [1],
				[Define to 1 if you have acl_get_file() and friends.])
	])
])

AC_ARG_ENABLE([libattr],
	AS_HELP_STRING([--disable-libattr],
		[Disable use of libattr (default: autodetect)]))

AS_IF([test x"$enable_libattr" != x"no"],
[
	AC_SEARCH_LIBS([getxattr], [attr],
	[
		AC_DEFINE([HAVE_LIBATTR], [1],
				[Define to 1 if you have getxattr() and friends.])
	])
])

AC_ARG_ENABLE([libcap],
	AS_HELP_STRING([--disable-libcap],
		[Disable use of libcap (default: autodetect)]))

AS_IF([test x"$enable_libcap" != x"no"],
[
	AC_SEARCH_LIBS([cap_get_file], [cap],
	[
		AC_DEFINE([HAVE_LIBCAP], [1],
				[Define to 1 if you have cap_get_file() and friends.])
	])
])

AC_CHECK_FUNCS([acl_get_link_np fchmodat lchown posix_fallocate utimensat])

AS_IF([test x"$enable_debug" = x"yes"],
[
	AC_DEFINE([ENABLE_DEBUG], [1], [Enable debugging code])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
