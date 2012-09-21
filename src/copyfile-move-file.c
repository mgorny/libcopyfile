/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

copyfile_error_t copyfile_move_file(const char* source,
		const char* dest, unsigned int* result_flags,
		copyfile_callback_t callback, void* callback_data)
{
	copyfile_error_t ret;
	copyfile_progress_t progress;

	progress.move.source = source;

	if (callback && callback(COPYFILE_NO_ERROR, COPYFILE_MOVE,
				progress, callback_data, 0))
		return COPYFILE_ABORTED;

	while (1)
	{
		if (!rename(source, dest))
		{
			if (result_flags)
				*result_flags = COPYFILE_COPY_ALL_METADATA;

			if (callback && callback(COPYFILE_EOF, COPYFILE_MOVE,
					progress, callback_data, 0))
				return COPYFILE_ABORTED;

			return COPYFILE_NO_ERROR;
		}
		else if (callback)
		{
			if (callback(COPYFILE_ERROR_RENAME, COPYFILE_MOVE,
						progress, callback_data,
						errno != EXDEV))
				return COPYFILE_ERROR_RENAME;
			else if (errno == EXDEV)
				break;
		}
		else /* default error handling */
		{
			if (errno == EXDEV)
				break;
			else
				return COPYFILE_ERROR_RENAME;
		}
	}

	while (unlink(dest) && errno != ENOENT)
	{
		if (callback)
		{
			if (callback(COPYFILE_ERROR_UNLINK_DEST, COPYFILE_MOVE,
						progress, callback_data, 1))
				return COPYFILE_ERROR_UNLINK_DEST;
		}
		else
			return COPYFILE_ERROR_UNLINK_DEST;
	}

	ret = copyfile_archive_file(source, dest, 0,
			COPYFILE_COPY_ALL_METADATA, result_flags,
			callback, callback_data);

	if (!ret)
	{
		while (unlink(source))
		{
			if (callback)
			{
				if (callback(COPYFILE_ERROR_UNLINK_SOURCE,
							COPYFILE_MOVE, progress, callback_data, 1))
					ret = COPYFILE_ERROR_UNLINK_SOURCE;
			}
			else
				ret = COPYFILE_ERROR_UNLINK_SOURCE;
		}
	}

	return ret;
}
