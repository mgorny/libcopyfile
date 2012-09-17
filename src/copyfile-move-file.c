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

	if (!rename(source, dest))
	{
		if (result_flags)
			*result_flags = COPYFILE_COPY_ALL_METADATA;
		/* XXX: callback */
		return COPYFILE_NO_ERROR;
	}
	else if (errno != EXDEV)
		return COPYFILE_ERROR_RENAME;

	if (unlink(dest) && errno != ENOENT)
		return COPYFILE_ERROR_UNLINK_DEST;

	ret = copyfile_archive_file(source, dest, 0,
			COPYFILE_COPY_ALL_METADATA, result_flags,
			callback, callback_data);

	if (!ret)
	{
		if (unlink(source))
			return COPYFILE_ERROR_UNLINK_SOURCE;
	}

	return ret;
}
