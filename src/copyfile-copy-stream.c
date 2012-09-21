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
#include <unistd.h>
#include <errno.h>

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
				if (callback(COPYFILE_NO_ERROR, COPYFILE_REGULAR,
							progress, callback_data))
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
					? !callback(err, COPYFILE_REGULAR, progress,
						callback_data)
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
						? !callback(err, COPYFILE_REGULAR, progress,
							callback_data)
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
	if (callback && callback(COPYFILE_EOF, COPYFILE_REGULAR, progress,
				callback_data))
		return COPYFILE_ABORTED;
	return COPYFILE_NO_ERROR;
}
