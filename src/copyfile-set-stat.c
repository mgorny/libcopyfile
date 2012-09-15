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
#include <errno.h>

copyfile_error_t copyfile_set_stat(const char* path,
		const struct stat* st, unsigned int flags,
		unsigned int* result_flags)
{
	assert(st);

	if (!flags)
		flags = COPYFILE_COPY_STAT;
	if (result_flags)
		*result_flags = 0;

	if (flags & COPYFILE_COPY_OWNER)
	{
		uid_t new_user = flags & COPYFILE_COPY_USER
			? st->st_uid : -1;
		gid_t new_group = flags & COPYFILE_COPY_GROUP
			? st->st_gid : -1;

#ifndef HAVE_LCHOWN
		/* don't try to chown() a symbolic link */
		if (!S_ISLNK(st->st_mode))
#endif
		{
#ifdef HAVE_LCHOWN
			if (lchown(path, new_user, new_group)
					&& errno != EOPNOTSUPP)
				return COPYFILE_ERROR_CHOWN;
#else /*!HAVE_LCHOWN*/
			if (chown(path, new_user, new_group))
				return COPYFILE_ERROR_CHOWN;
#endif /*HAVE_LCHOWN*/

			if (result_flags)
				*result_flags |= flags & COPYFILE_COPY_OWNER;
		}
	}

	if (flags & COPYFILE_COPY_MODE)
	{
#ifndef HAVE_FCHMODAT
		/* don't try to chmod() a symbolic link */
		if (!S_ISLNK(st->st_mode))
#endif /*HAVE_FCHMODAT*/
		{
#ifdef HAVE_FCHMODAT

			int at_flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;

			if (!fchmodat(AT_FDCWD, path, st->st_mode & all_perm_bits,
						at_flags))
			{
				if (result_flags)
					*result_flags |= COPYFILE_COPY_MODE;
			}
			else if (errno != EOPNOTSUPP)
				return COPYFILE_ERROR_CHMOD;

#else /*!HAVE_FCHMODAT*/

			if (!chmod(path, st->st_mode & all_perm_bits))
			{
				if (result_flags)
					*result_flags |= COPYFILE_COPY_MODE;
			}
			else
				return COPYFILE_ERROR_CHMOD;

#endif /*HAVE_FCHMODAT*/
		}
	}

	if (flags & COPYFILE_COPY_TIMES)
	{
#ifndef HAVE_UTIMENSAT
#	ifndef HAVE_LUTIMES
		if (S_ISLNK(st->st_mode))
			return COPYFILE_NO_ERROR;
#	endif
#endif

#ifdef HAVE_UTIMES
		{
			struct timespec t[2];

#ifdef HAVE_UTIMENSAT
			int at_flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;
#endif

#ifdef HAVE_STRUCT_STAT_ST_ATIMESPEC /* BSD */
			t[0] = st->st_atimespec;
			t[1] = st->st_mtimespec;
#else /*!HAVE_STRUCT_STAT_ST_ATIMESPEC*/
			t[0] = st->st_atim;
			t[1] = st->st_mtim;
#endif /*HAVE_STRUCT_STAT_ST_ATIMESPEC*/

#ifdef HAVE_UTIMENSAT

			if (!(flags & COPYFILE_COPY_ATIME))
				t[0].tv_nsec = UTIME_OMIT;

			if (!(flags & COPYFILE_COPY_MTIME))
				t[1].tv_nsec = UTIME_OMIT;

			if (!utimensat(AT_FDCWD, path, t, at_flags))
			{
				if (result_flags)
					*result_flags |= (flags & COPYFILE_COPY_TIMES);
			}
			else if (errno != EOPNOTSUPP)
				return COPYFILE_ERROR_UTIME;

#else /*!HAVE_UTIMENSAT*/
			{
				struct timeval tv[2];
				int ret;

				tv[0].tv_sec = t[0].tv_sec;
				tv[0].tv_usec = t[0].tv_nsec / 1000;
				tv[1].tv_sec = t[1].tv_sec;
				tv[1].tv_usec = t[1].tv_nsec / 1000;

#ifdef HAVE_LUTIMES
				ret = lutimes(path, tv);
#else
				ret = utimes(path, tv);
#endif
				if (!ret)
				{
					if (result_flags)
						*result_flags |= COPYFILE_COPY_TIMES;
				}
				else
					return COPYFILE_ERROR_UTIME;
			}
#endif /*HAVE_UTIMENSAT*/
		}
#else /*!HAVE_UTIMES*/
		{
			struct utimbuf t;

			t.actime = st->st_atime;
			t.modtime = st->st_mtime;

			if (utime(path, &t))
				return COPYFILE_ERROR_UTIME;

			if (result_flags)
				*result_flags |= COPYFILE_COPY_TIMES;
		}
#endif /*HAVE_UTIMENSAT*/
	}

	return COPYFILE_NO_ERROR;
}
