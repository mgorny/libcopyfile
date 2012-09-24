/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
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
	int open_flags = O_WRONLY | O_CREAT;

	/* if there's no ftruncate(), we need to truncate when opening.
	 * if there's posix_fallocate(), we truncate anyway trying to get
	 * a less fragmented space. */
#ifndef HAVE_FTRUNCATE
	open_flags |= O_TRUNC;
#endif
#ifdef HAVE_POSIX_FALLOCATE
	open_flags |= O_TRUNC;
#endif

	fd_in = open(source, O_RDONLY);
	if (fd_in == -1)
		return COPYFILE_ERROR_OPEN_SOURCE;

	fd_out = open(dest, open_flags, perm_file);
	if (fd_out == -1)
	{
		int hold_errno = errno;

		close(fd_in);

		errno = hold_errno;
		return COPYFILE_ERROR_OPEN_DEST;
	}

	/* we can't use posix_fallocate() if we wouldn't be able to truncate
	 * afterwards. not that any system can really have former without
	 * the latter; but since autotools does the check already, we can
	 * use it here too. */
#ifdef HAVE_POSIX_FALLOCATE
#	ifdef HAVE_FTRUNCATE

	if (expected_size && !posix_fallocate(fd_out, 0, expected_size))
		++preallocated;

#	endif /*HAVE_FTRUNCATE*/
#endif /*HAVE_POSIX_FALLOCATE*/

	{
		off_t offset = 0;
		copyfile_error_t ret = copyfile_copy_stream(fd_in, fd_out,
				&offset, expected_size, callback, callback_data);
		int hold_errno = errno;

#ifdef HAVE_FTRUNCATE
		if (ftruncate(fd_out, offset) && !ret)
		{
			ret = COPYFILE_ERROR_TRUNCATE;
			hold_errno = errno;
		}
#endif

		if (close(fd_out) && !ret) /* delayed error? */
			return COPYFILE_ERROR_WRITE;
		close(fd_in);

		errno = hold_errno;
		return ret;
	}
}
