/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>

static unsigned int copy_owner(const char* path,
		const struct stat* st, unsigned int flags)
{
#ifdef HAVE_CHOWN

	uid_t new_user = flags & COPYFILE_COPY_USER
		? st->st_uid : -1;
	gid_t new_group = flags & COPYFILE_COPY_GROUP
		? st->st_gid : -1;

#	ifdef HAVE_LCHOWN

	if (!lchown(path, new_user, new_group))
		return flags & COPYFILE_COPY_OWNER;

#	else /*!HAVE_LCHOWN*/

#		ifdef S_IFLNK

	/* don't try to chown() a symbolic link */
	if (S_ISLNK(st->st_mode))
		return 0;

#		endif /*S_IFLNK*/

	if (!chown(path, new_user, new_group))
		return flags & COPYFILE_COPY_OWNER;

#	endif /*HAVE_LCHOWN*/

#endif /*HAVE_CHOWN*/

	return 0;
}

static unsigned int copy_mode(const char* path,
		const struct stat* st, unsigned int flags)
{
#ifdef HAVE_FCHMODAT

#	ifdef S_IFLNK

	int at_flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;

#	else /*!S_IFLNK*/

	int at_flags = 0;

#	endif /*S_IFLNK*/

	if (!fchmodat(AT_FDCWD, path, st->st_mode & all_perm_bits,
				at_flags))
		return COPYFILE_COPY_MODE;

#else /*!HAVE_FCHMODAT*/

#	ifdef S_IFLNK

	/* don't try to chmod() a symbolic link */
	if (S_ISLNK(st->st_mode))
		return 0;

#	endif /*S_IFLNK*/

	if (!chmod(path, st->st_mode & all_perm_bits))
		return COPYFILE_COPY_MODE;

#endif /*HAVE_FCHMODAT*/

	return 0;
}

static unsigned int copy_times(const char* path,
		const struct stat* st, unsigned int flags)
{
#ifndef HAVE_UTIMENSAT
#	ifndef HAVE_LUTIMES
#		ifdef S_IFLNK

	if (S_ISLNK(st->st_mode))
		return 0;

#		endif /*S_IFLNK*/
#	endif /*HAVE_LUTIMES*/
#endif /*HAVE_UTIMENSAT*/

#ifdef HAVE_UTIMES

	{
		struct timespec t[2];

#	ifdef HAVE_STRUCT_STAT_ST_ATIMESPEC /* BSD */
		t[0] = st->st_atimespec;
		t[1] = st->st_mtimespec;
#	else /*!HAVE_STRUCT_STAT_ST_ATIMESPEC*/
		t[0] = st->st_atim;
		t[1] = st->st_mtim;
#	endif /*HAVE_STRUCT_STAT_ST_ATIMESPEC*/

#	ifdef HAVE_UTIMENSAT

		if (!(flags & COPYFILE_COPY_ATIME))
			t[0].tv_nsec = UTIME_OMIT;
		if (!(flags & COPYFILE_COPY_MTIME))
			t[1].tv_nsec = UTIME_OMIT;

		{
#	ifdef S_IFLNK
			int at_flags = S_ISLNK(st->st_mode)
				? AT_SYMLINK_NOFOLLOW : 0;
#	else /*!S_IFLNK*/
			int at_flags = 0;
#	endif /*S_IFLNK*/

			if (!utimensat(AT_FDCWD, path, t, at_flags))
				return (flags & COPYFILE_COPY_TIMES);
		}

#	else /*!HAVE_UTIMENSAT*/

		{
			struct timeval tv[2];

			tv[0].tv_sec = t[0].tv_sec;
			tv[0].tv_usec = t[0].tv_nsec / 1000;
			tv[1].tv_sec = t[1].tv_sec;
			tv[1].tv_usec = t[1].tv_nsec / 1000;

#		ifdef HAVE_LUTIMES
			if (!lutimes(path, tv))
				return COPYFILE_COPY_TIMES;
#		else
			if (!utimes(path, tv))
				return COPYFILE_COPY_TIMES;
#		endif
		}

#	endif /*HAVE_UTIMENSAT*/

	}

#else /*!HAVE_UTIMES*/

	{
		struct utimbuf t;

		t.actime = st->st_atime;
		t.modtime = st->st_mtime;

		if (!utime(path, &t))
			return COPYFILE_COPY_TIMES;
	}

#endif /*HAVE_UTIMENSAT*/

	return 0;
}

unsigned int copyfile_set_stat(const char* path,
		const struct stat* st, unsigned int flags)
{
	unsigned int ret = 0;

	assert(st);

	if (!flags)
		flags = COPYFILE_COPY_STAT;

	if (flags & COPYFILE_COPY_OWNER)
		ret |= copy_owner(path, st, flags);

	if (flags & COPYFILE_COPY_MODE)
		ret |= copy_mode(path, st, flags);

	if (flags & COPYFILE_COPY_TIMES)
		ret |= copy_times(path, st, flags);

	return ret;
}
