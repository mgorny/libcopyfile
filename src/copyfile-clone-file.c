/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
#include "common.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

copyfile_error_t copyfile_clone_file(const char* source,
		const char* dest, const struct stat* st)
{
	struct stat buf;
	mode_t ftype;

	if (!st)
	{
#ifdef S_IFLNK
		if (lstat(source, &buf))
			return COPYFILE_ERROR_STAT;
#else
		if (stat(source, &buf))
			return COPYFILE_ERROR_STAT;
#endif

		st = &buf;
	}

	ftype = st->st_mode & S_IFMT;

	if (ftype == S_IFREG)
	{
		int fd_in, fd_out;
		int ret, ret_errno;

		fd_in = open(source, O_RDONLY);
		if (fd_in == -1)
			return COPYFILE_ERROR_OPEN_SOURCE;

		fd_out = open(dest, O_WRONLY|O_CREAT, perm_file);
		if (fd_out == -1)
		{
			int hold_errno = errno;

			close(fd_in);

			errno = hold_errno;
			return COPYFILE_ERROR_OPEN_DEST;
		}

		ret = copyfile_clone_stream(fd_in, fd_out);
		ret_errno = errno;

		close(fd_in);
		if (close(fd_out) && !ret) /* delayed error? */
			return COPYFILE_ERROR_WRITE;

		errno = ret_errno;
		return ret;
	}
	else
		return COPYFILE_ERROR_UNSUPPORTED;
}
