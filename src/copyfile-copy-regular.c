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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
