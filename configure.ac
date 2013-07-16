AC_PREREQ([2.60])
AC_INIT([libcopyfile], [0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2 subdir-objects])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([disable-static])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [Enable stderr debug]))

AC_ARG_ENABLE([acl],
	AS_HELP_STRING([--disable-acl],
		[Disable support for ACLs (default: autodetect)]))

AS_IF([test x"$enable_acl" != x"no"],
[
	AC_SEARCH_LIBS([acl_get_file], [acl],
	[
		AC_DEFINE([HAVE_LIBACL], [1],
				[Define to 1 if you have acl_get_file() and friends.])
	])
])

AC_ARG_ENABLE([xattr],
	AS_HELP_STRING([--disable-xattr],
		[Disable support for file extended attributes (default: autodetect)]))

AS_IF([test x"$enable_xattr" != x"no"],
[
	AC_CHECK_FUNCS([lgetxattr extattr_get_link],
	[
		AC_DEFINE([HAVE_XATTR], [1],
				[Define to 1 if any xattr support is available.])
		break
	])
])

AC_ARG_ENABLE([cap],
	AS_HELP_STRING([--disable-cap],
		[Disable support for capabilities (default: autodetect)]))

AS_IF([test x"$enable_cap" != x"no"],
[
	AC_SEARCH_LIBS([cap_get_file], [cap],
	[
		AC_DEFINE([HAVE_LIBCAP], [1],
				[Define to 1 if you have cap_get_file() and friends.])
	])
])

AC_CHECK_FUNCS([acl_get_link_np chown fchmodat ftruncate lchown \
	link mkfifo mknod posix_fallocate lutimes utimes utimensat \
	getopt_long])
AC_CHECK_MEMBERS([struct stat.st_atimespec])

AS_IF([test x"$enable_debug" = x"yes"],
[
	AC_DEFINE([ENABLE_DEBUG], [1], [Enable debugging code])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile data/libcopyfile.pc])
AC_OUTPUT
