/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"

#include <stddef.h> /* size_t */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#ifndef COPYFILE_BUFFER_SIZE
#	define COPYFILE_BUFFER_SIZE 4096
#endif

#ifndef COPYFILE_CALLBACK_OPCOUNT
#	define COPYFILE_CALLBACK_OPCOUNT 64
#endif

copyfile_error_t copyfile_copy_stream(int fd_in, int fd_out,
		copyfile_callback_t callback, void* callback_data)
{
	off_t in_pos = 0;
	char buf[COPYFILE_BUFFER_SIZE];

	int opcount = 0;

	while (1)
	{
		char* bufp = buf;
		ssize_t rd, wr;

		if (callback)
		{
			if (++opcount >= COPYFILE_CALLBACK_OPCOUNT)
			{
				if (callback(COPYFILE_NO_ERROR, in_pos, callback_data))
					return COPYFILE_ABORTED;
				opcount = 0;
			}
		}

		rd = read(fd_in, bufp, sizeof(buf));
		if (rd == -1)
		{
			copyfile_error_t err = COPYFILE_ERROR_READ;

			if (callback
					? !callback(err, in_pos, callback_data)
					: errno == EINTR)
				continue;
			else
				return err;
		}
		else if (rd == 0)
			break;

		in_pos += rd;

		while (rd > 0)
		{
			wr = write(fd_out, bufp, rd);
			if (wr == -1)
			{
				copyfile_error_t err = COPYFILE_ERROR_WRITE;

				if (callback
						? !callback(err, in_pos, callback_data)
						: errno == EINTR)
					continue;
				else
					return err;
			}
			else
			{
				rd -= wr;
				bufp += wr;
			}
		}
	}

	if (callback && callback(COPYFILE_EOF, in_pos, callback_data))
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

	fd_out = creat(dest, 0666);
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
		copyfile_error_t ret
			= copyfile_copy_stream(fd_in, fd_out, callback, callback_data);
		int hold_errno = errno;

#ifdef HAVE_POSIX_FALLOCATE
		if (preallocated)
		{
			off_t pos = lseek(fd_out, 0, SEEK_CUR);
			if (pos != -1)
			{
				int trunc_ret = ftruncate(fd_out, pos);
				if (trunc_ret && !ret)
				{
					ret = COPYFILE_ERROR_TRUNCATE;
					hold_errno = trunc_ret;
				}
			}
			else if (!ret)
			{
				ret = COPYFILE_ERROR_TRUNCATE;
				hold_errno = errno;
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
