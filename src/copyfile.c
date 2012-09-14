/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <utime.h>

#ifdef HAVE_LIBATTR
#	include <attr/xattr.h>
#endif
#ifdef HAVE_LIBACL
#	include <sys/acl.h>
#endif

#ifndef COPYFILE_BUFFER_SIZE
#	define COPYFILE_BUFFER_SIZE 4096
#endif

#ifndef COPYFILE_CALLBACK_OPCOUNT
#	define COPYFILE_CALLBACK_OPCOUNT 64
#endif

static const int not_reached = 0;

/* default permissions */
static const mode_t perm_dir = S_IRWXU | S_IRWXG | S_IRWXO;
static const mode_t perm_file = perm_dir
		& ~(S_IXUSR | S_IXGRP | S_IXOTH);
static const mode_t all_perm_bits = perm_dir
		| S_ISUID | S_ISGID | S_ISVTX;

copyfile_error_t copyfile_copy_stream(int fd_in, int fd_out,
		off_t* offset_store, off_t expected_size,
		copyfile_callback_t callback, void* callback_data)
{
	char buf[COPYFILE_BUFFER_SIZE];

	int opcount = 0;
	copyfile_progress_t progress;

	progress.data.offset = offset_store ? *offset_store : 0;
	progress.data.size = expected_size;

	while (1)
	{
		char* bufp = buf;
		ssize_t rd, wr;

		if (callback)
		{
			if (++opcount >= COPYFILE_CALLBACK_OPCOUNT)
			{
				if (callback(COPYFILE_NO_ERROR, S_IFREG, progress, callback_data))
				{
					if (offset_store)
						*offset_store = progress.data.offset;
					return COPYFILE_ABORTED;
				}
				opcount = 0;
			}
		}

		rd = read(fd_in, bufp, sizeof(buf));
		if (rd == -1)
		{
			copyfile_error_t err = COPYFILE_ERROR_READ;

			if (callback
					? !callback(err, S_IFREG, progress, callback_data)
					: errno == EINTR)
				continue;
			else
			{
				if (offset_store)
					*offset_store = progress.data.offset;
				return err;
			}
		}
		else if (rd == 0)
			break;

		while (rd > 0)
		{
			wr = write(fd_out, bufp, rd);
			if (wr == -1)
			{
				copyfile_error_t err = COPYFILE_ERROR_WRITE;

				if (callback
						? !callback(err, S_IFREG, progress, callback_data)
						: errno == EINTR)
					continue;
				else
				{
					if (offset_store)
						*offset_store = progress.data.offset;
					return err;
				}
			}
			else
			{
				rd -= wr;
				bufp += wr;

				progress.data.offset += wr;
			}
		}
	}

	if (offset_store)
		*offset_store = progress.data.offset;
	if (callback && callback(COPYFILE_EOF, S_IFREG, progress, callback_data))
		return COPYFILE_ABORTED;
	return COPYFILE_NO_ERROR;
}

copyfile_error_t copyfile_copy_regular(const char* source,
		const char* dest, off_t expected_size,
		copyfile_callback_t callback, void* callback_data)
{
	int fd_in, fd_out;
#ifdef HAVE_POSIX_FALLOCATE
	int preallocated = 0;
#endif

	fd_in = open(source, O_RDONLY);
	if (fd_in == -1)
		return COPYFILE_ERROR_OPEN_SOURCE;

	fd_out = creat(dest, perm_file);
	if (fd_out == -1)
	{
		int hold_errno = errno;

		close(fd_in);

		errno = hold_errno;
		return COPYFILE_ERROR_OPEN_DEST;
	}

#ifdef HAVE_POSIX_FALLOCATE
	if (expected_size && !posix_fallocate(fd_out, 0, expected_size))
		++preallocated;
#endif

	{
		off_t offset = 0;
		copyfile_error_t ret = copyfile_copy_stream(fd_in, fd_out,
				&offset, expected_size, callback, callback_data);
		int hold_errno = errno;

#ifdef HAVE_POSIX_FALLOCATE
		if (preallocated)
		{
			int trunc_ret = ftruncate(fd_out, offset);
			if (trunc_ret && !ret)
			{
				ret = COPYFILE_ERROR_TRUNCATE;
				hold_errno = trunc_ret;
			}
		}
#endif

		if (close(fd_out) && !ret) /* delayed error? */
			return COPYFILE_ERROR_WRITE;
		close(fd_in);

		errno = hold_errno;
		return ret;
	}
}

static copyfile_error_t try_copy_symlink(const char* source,
		const char* dest, char* buf, ssize_t buf_size)
{
	ssize_t rd = readlink(source, buf, buf_size);

	if (rd == -1)
		return COPYFILE_ERROR_READLINK;

	if (rd < buf_size)
	{
		buf[rd + 1] = 0;

		if (symlink(buf, dest))
			return COPYFILE_ERROR_SYMLINK;
		return COPYFILE_NO_ERROR;
	}

	return COPYFILE_EOF;
}

copyfile_error_t copyfile_copy_symlink(const char* source,
		const char* dest, size_t expected_length,
		copyfile_callback_t callback, void* callback_data)
{
	/* remember that the last cell is for null-terminator */

	/* try to avoid dynamic allocation */
	char buf[COPYFILE_BUFFER_SIZE];
	size_t buf_size = sizeof(buf);

	copyfile_progress_t progress;

	progress.symlink.length = expected_length;
	if (callback && callback(COPYFILE_NO_ERROR, S_IFLNK, progress, callback_data))
		return COPYFILE_ABORTED;

	if (expected_length < buf_size)
	{
		int ret;

		while (1)
		{
			ret = try_copy_symlink(source, dest, buf, buf_size);

			if (ret != COPYFILE_EOF)
			{
				if (!ret)
				{
					if (callback)
					{
						progress.symlink.target = buf;
						if (callback(COPYFILE_EOF, S_IFLNK, progress,
									callback_data))
							return COPYFILE_ABORTED;
					}

					return ret;
				}

				/* does the user want to retry? */
				if (!callback || callback(ret, S_IFLNK, progress,
							callback_data))
					return ret;
			}
			else
				break;
		}

		buf_size *= 2;
	}
	else
		buf_size = expected_length + 1;

	{
		char* buf = 0;
		int ret;

		/* size_t is always unsigned, so -1 safely becomes SIZE_MAX */
		const size_t my_size_max = -1;
		const size_t max_size = my_size_max > SSIZE_MAX
			? SSIZE_MAX : my_size_max;

		while (1)
		{
			char* next_buf = realloc(buf, buf_size);

			if (!next_buf)
			{
				ret = COPYFILE_ERROR_MALLOC;

				/* retry? */
				if (callback && !callback(ret, S_IFLNK, progress, callback_data))
					continue;

				if (!buf) /* avoid freeing in the less likely branch */
					return ret;
				break;
			}
			buf = next_buf;

			ret = try_copy_symlink(source, dest, buf, buf_size);

			if (!ret)
			{
				if (callback)
				{
					progress.symlink.target = buf;
					if (callback(COPYFILE_EOF, S_IFLNK, progress,
								callback_data))
						return COPYFILE_ABORTED;
				}
				break;
			}
			else if (ret == COPYFILE_EOF)
			{
				if (buf_size < max_size / 2)
					buf_size *= 2;
				else
				{
					if (buf_size != max_size)
						buf_size = max_size;
					else
					{
						ret = COPYFILE_ERROR_SYMLINK_TARGET_TOO_LONG;
						break;
					}
				}
			}
			else
			{
				/* does the user want to retry? */
				if (!callback && callback(ret, S_IFLNK, progress, callback_data))
					break;
			}
		}

		free(buf);
		return ret;
	}
}

copyfile_error_t copyfile_create_special(const char* path, mode_t ftype,
		dev_t devid, copyfile_callback_t callback, void* callback_data)
{
	copyfile_progress_t progress;

	switch (ftype)
	{
		case S_IFBLK:
		case S_IFCHR:
			progress.device = devid;
			break;
	}

	if (callback && callback(COPYFILE_NO_ERROR, ftype, progress,
				callback_data))
		return COPYFILE_ABORTED;

	while (1)
	{
		int ret;
		copyfile_error_t err;

		switch (ftype)
		{
			case S_IFDIR:
				ret = mkdir(path, perm_dir);
				err = COPYFILE_ERROR_MKDIR;
				break;
			case S_IFIFO:
				ret = mkfifo(path, perm_file);
				err = COPYFILE_ERROR_MKFIFO;
				break;
			case S_IFBLK:
			case S_IFCHR:
				ret = mknod(path, ftype | perm_file, devid);
				err = COPYFILE_ERROR_MKNOD;
				break;
			case S_IFSOCK:
				{
					int fd;
					struct sockaddr_un addr;

					const size_t path_size = strlen(path) + 1;

					if (path_size > sizeof(addr.sun_path))
						return COPYFILE_ERROR_SOCKET_DEST_TOO_LONG;

					fd = socket(AF_UNIX, SOCK_STREAM, 0);
					if (fd != -1)
					{
						addr.sun_family = AF_UNIX;
						memcpy(addr.sun_path, path, path_size);

						ret = bind(fd, &addr, sizeof(addr));
						err = COPYFILE_ERROR_BIND;

						close(fd);
					}
					else
					{
						ret = fd;
						err = COPYFILE_ERROR_SOCKET;
					}
				}
				break;
			default:
				assert(not_reached);
				return COPYFILE_ERROR_INTERNAL;
		}

		if (!ret)
			break;
		else
		{
			if (!callback || callback(err, ftype, progress,
						callback_data))
				return err;
		}
	}

	if (callback && callback(COPYFILE_EOF, ftype, progress,
				callback_data))
		return COPYFILE_ABORTED;

	return COPYFILE_NO_ERROR;
}

copyfile_error_t copyfile_copy_file(const char* source,
		const char* dest, const struct stat* st,
		copyfile_callback_t callback, void* callback_data)
{
	struct stat buf;
	mode_t ftype;

	if (!st)
	{
		if (lstat(source, &buf))
			return COPYFILE_ERROR_STAT;

		st = &buf;
	}

	ftype = st->st_mode & S_IFMT;

	switch (ftype)
	{
		case S_IFREG:
			return copyfile_copy_regular(source, dest, st->st_size,
					callback, callback_data);
		case S_IFLNK:
			return copyfile_copy_symlink(source, dest, st->st_size,
					callback, callback_data);
		default:
			return copyfile_create_special(dest, ftype, st->st_rdev,
					callback, callback_data);
	}
}

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
#else
			if (chown(path, new_user, new_group))
				return COPYFILE_ERROR_CHOWN;
#endif

			if (result_flags)
				*result_flags |= flags & COPYFILE_COPY_OWNER;
		}
	}

	if (flags & COPYFILE_COPY_MODE)
	{
#ifndef HAVE_FCHMODAT
		/* don't try to chmod() a symbolic link */
		if (!S_ISLNK(st->st_mode))
#endif
		{
#ifdef HAVE_FCHMODAT

			int flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;

			if (!fchmodat(AT_FDCWD, path, st->st_mode & all_perm_bits,
						flags))
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

#endif
		}
	}

	if (flags & COPYFILE_COPY_TIMES)
	{
#ifdef HAVE_UTIMENSAT
		{
			struct timespec t[2];
			int flags = S_ISLNK(st->st_mode) ? AT_SYMLINK_NOFOLLOW : 0;

			if (flags & COPYFILE_COPY_ATIME)
				t[0] = st->st_atim;
			else
				t[0].tv_nsec = UTIME_OMIT;

			if (flags & COPYFILE_COPY_MTIME)
				t[1] = st->st_mtim;
			else
				t[1].tv_nsec = UTIME_OMIT;

			if (!utimensat(AT_FDCWD, path, t, flags))
			{
				if (result_flags)
					*result_flags |= (flags & COPYFILE_COPY_TIMES);
			}
			else if (errno != EOPNOTSUPP)
				return COPYFILE_ERROR_UTIME;
		}
#else /*!HAVE_UTIMENSAT*/
		/* don't try utime() on a symbolic link */
		if (!S_ISLNK(st->st_mode))
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

copyfile_error_t copyfile_copy_xattr(const char* source,
		const char* dest, unsigned int flags,
		unsigned int* result_flags)
{
	if (result_flags)
		*result_flags = 0;

#ifdef HAVE_LIBATTR
	if (!flags)
		flags = COPYFILE_COPY_XATTR_ALL;

	/* sadly, we can't use attr_copy_file() because it doesn't provide
	 * any good way to distinguish between read and write errors. */
	if (flags & COPYFILE_COPY_XATTR_ALL)
	{
		char list_buf[COPYFILE_BUFFER_SIZE];
		char data_buf[COPYFILE_BUFFER_SIZE];
		const ssize_t initial_buf_size = sizeof(list_buf);

		char* list_bufp = list_buf;
		ssize_t list_buf_size = initial_buf_size;

		char* data_bufp = data_buf;
		ssize_t data_buf_size = initial_buf_size;

		ssize_t list_len;

		const char* n;

		copyfile_error_t ret = COPYFILE_NO_ERROR;
		int saved_errno;

		list_len = llistxattr(source, 0, 0);
		if (list_len == -1)
		{
			/* if source fs doesn't support them, it doesn't have them. */
			if (errno == ENOTSUP)
				return COPYFILE_NO_ERROR;
			else
				return COPYFILE_ERROR_XATTR_LIST;
		}

		if (list_len > list_buf_size)
		{
			list_bufp = malloc(list_len);
			if (!list_bufp)
				return COPYFILE_ERROR_MALLOC;
			list_buf_size = list_len;
		}

		list_len = llistxattr(source, list_bufp, list_buf_size);
		if (list_len == -1)
		{
			if (list_buf_size > initial_buf_size)
				free(list_bufp);

			return COPYFILE_ERROR_XATTR_LIST;
		}

		for (n = list_bufp; n < &list_bufp[list_len]; n = strchr(n, 0) + 1)
		{
			ssize_t data_len;
			unsigned int special_bit = 0;

			if (!strcmp(n, "system.posix_acl_access"))
				special_bit = COPYFILE_COPY_ACL_ACCESS;
			else if (!strcmp(n, "system.posix_acl_default"))
				special_bit = COPYFILE_COPY_ACL_DEFAULT;

			/* omit special flags if not requested */
			if (special_bit && !(flags & special_bit))
				continue;

			data_len = lgetxattr(source, n, 0, 0);
			if (data_len == -1)
			{
				/* return the first error
				 * but try to copy the remaining attributes first,
				 * in case user ignored errors */
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_GET;
					saved_errno = errno;
				}
				continue;
			}

			if (data_len > data_buf_size)
			{
				char* new_data_bufp = realloc(data_bufp, data_len);
				if (!new_data_bufp)
				{
					if (!ret)
					{
						ret = COPYFILE_ERROR_MALLOC;
						saved_errno = errno;
					}
					continue;
				}
				data_bufp = new_data_bufp;
				data_buf_size = data_len;
			}

			data_len = lgetxattr(source, n, data_bufp, data_buf_size);
			if (data_len == -1)
			{
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_GET;
					saved_errno = errno;
				}
				continue;
			}

			if (lsetxattr(dest, n, data_bufp, data_len, 0))
			{
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_SET;
					saved_errno = errno;
				}

				/* further calls will fail as well */
				if (errno == ENOTSUP)
					break;
			}
			else if (special_bit && result_flags)
				*result_flags |= special_bit;
		}

		if (list_buf_size > initial_buf_size)
			free(list_bufp);
		if (data_buf_size > initial_buf_size)
			free(data_bufp);

		if (ret)
			errno = saved_errno;
		else if (result_flags)
			*result_flags |= COPYFILE_COPY_XATTR;
		return ret;
	}
#endif /*HAVE_LIBATTR*/

	return COPYFILE_ERROR_UNSUPPORTED;
}

#ifdef HAVE_LIBACL
static const acl_type_t acl_types[] =
{
	ACL_TYPE_ACCESS,
	ACL_TYPE_DEFAULT
};

static const unsigned int acl_flags[] =
{
	COPYFILE_COPY_ACL_ACCESS,
	COPYFILE_COPY_ACL_DEFAULT
};
#endif /*HAVE_LIBACL*/

copyfile_error_t copyfile_copy_acl(const char* source,
		const char* dest, unsigned int flags,
		unsigned int* result_flags)
{
	if (result_flags)
		*result_flags = 0;

#ifdef HAVE_LIBACL
	if (!flags)
		flags = COPYFILE_COPY_ACL;

	{
		copyfile_error_t ret = COPYFILE_NO_ERROR;
		int saved_errno;

		int i;

		for (i = 0; i < 2; ++i)
		{
			if (flags & acl_flags[i])
			{
				acl_t acl;

				acl = acl_get_file(source, acl_types[i]);
				if (acl)
				{
					if (!acl_set_file(dest, acl_types[i], acl))
					{
						if (result_flags)
							*result_flags |= acl_flags[i];
					}
					else if (!ret)
					{
						ret = COPYFILE_ERROR_ACL_SET;
						saved_errno = errno;
					}

					acl_free(acl);
				}
				else
				{
					/* ACLs not supported? fine, nothing to copy. */
					if (errno == EOPNOTSUPP)
						return COPYFILE_NO_ERROR;
					else if (i > 0 && errno == EACCES)
						/* ACL_TYPE_DEFAULT on non-dir, likely */;
					else if (!ret)
					{
						ret = COPYFILE_ERROR_ACL_GET;
						saved_errno = errno;
					}
				}
			}
		}

		if (ret)
			errno = saved_errno;
		return ret;
	}
#endif /*HAVE_LIBACL*/

	return COPYFILE_ERROR_UNSUPPORTED;
}

copyfile_error_t copyfile_copy_metadata(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags)
{
	struct stat buf;
	unsigned int done = 0;

	if (!flags)
		flags = COPYFILE_COPY_ALL_METADATA;
	if (result_flags)
		*result_flags = 0;
	if (!st)
	{
		if (lstat(source, &buf))
			return COPYFILE_ERROR_STAT;

		st = &buf;
	}

	/* Now, order is important.
	 *
	 * 1) chown() because changing owners can change modes,
	 * 2) xattr because xattr can sometimes copy ACLs as well,
	 * 3) ACLs because copying ACLs implies chmod(),
	 * 4) remaining stat() stuff (including chmod() for special modes).
	 */

	if (flags & COPYFILE_COPY_OWNER)
	{
		copyfile_error_t ret = copyfile_set_stat(dest, st,
				flags & COPYFILE_COPY_OWNER, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;

		if (ret)
			return ret;
	}

	if (flags & COPYFILE_COPY_XATTR_ALL)
	{
		copyfile_copy_xattr(source, dest, flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_ACL)
	{
		copyfile_copy_acl(source, dest, flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_STAT)
	{
		copyfile_error_t ret = copyfile_set_stat(dest, st,
				flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;

		if (ret)
			return ret;
	}

	return COPYFILE_NO_ERROR;
}
