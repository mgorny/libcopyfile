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

unsigned int copyfile_set_stat(const char* path,
		const struct stat* st, unsigned int flags)
{
	unsigned int ret = 0;

	assert(st);

	if (!flags)
		flags = COPYFILE_COPY_STAT;

#ifdef HAVE_CHOWN

	if (flags & COPYFILE_COPY_OWNER)
	{
		uid_t new_user = flags & COPYFILE_COPY_USER
			? st->st_uid : -1;
		gid_t new_group = flags & COPYFILE_COPY_GROUP
			? st->st_gid : -1;

#	ifdef HAVE_LCHOWN

		if (!lchown(path, new_user, new_group))
			flags |= COPYFILE_COPY_OWNER;

#	else /*!HAVE_LCHOWN*/

		/* don't try to chown() a symbolic link */
		if (!S_ISLNK(st->st_mode))
		{
			if (!chown(path, new_user, new_group))
				flags |= COPYFILE_COPY_OWNER;
		}

#	endif /*HAVE_LCHOWN*/

	}

#endif /*HAVE_CHOWN*/

	if (flags & COPYFILE_COPY_MODE)
	{
#ifdef HAVE_FCHMODAT

		int at_flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;

		if (!fchmodat(AT_FDCWD, path, st->st_mode & all_perm_bits,
					at_flags))
			ret |= COPYFILE_COPY_MODE;

#else /*!HAVE_FCHMODAT*/

		/* don't try to chmod() a symbolic link */
		if (!S_ISLNK(st->st_mode))
		{
			if (!chmod(path, st->st_mode & all_perm_bits))
				ret |= COPYFILE_COPY_MODE;
		}

#endif /*HAVE_FCHMODAT*/
	}

	if (flags & COPYFILE_COPY_TIMES)
	{
#ifndef HAVE_UTIMENSAT
#	ifndef HAVE_LUTIMES
		if (!S_ISLNK(st->st_mode))
#	endif
#endif

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
				int at_flags = S_ISLNK(st->st_mode)
					? AT_SYMLINK_NOFOLLOW : 0;

				if (!utimensat(AT_FDCWD, path, t, at_flags))
					ret |= (flags & COPYFILE_COPY_TIMES);
			}

#	else /*!HAVE_UTIMENSAT*/

			{
				struct timeval tv[2];
				int ret;

				tv[0].tv_sec = t[0].tv_sec;
				tv[0].tv_usec = t[0].tv_nsec / 1000;
				tv[1].tv_sec = t[1].tv_sec;
				tv[1].tv_usec = t[1].tv_nsec / 1000;

#		ifdef HAVE_LUTIMES
				if (!lutimes(path, tv))
					ret |= COPYFILE_COPY_TIMES;
#		else
				if (!utimes(path, tv))
					ret |= COPYFILE_COPY_TIMES;
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
				ret |= COPYFILE_COPY_TIMES;
		}

#endif /*HAVE_UTIMENSAT*/

	}

	return ret;
}
