/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
#include "common.h"

#include <unistd.h>
#include <errno.h>

copyfile_error_t copyfile_link_file_dedup(const char* source,
		const char* dest, const char* dup_copy,
		const struct stat* st, unsigned int* result_flags,
		copyfile_callback_t callback, void* callback_data)
{
	struct stat buf;
	copyfile_error_t clone_ret;

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

	/* Try to clone the duplicate-candidate first. */
	if (!copyfile_clone_file(dup_copy, dest, st))
		return copyfile_copy_metadata(source, dest, st,
				COPYFILE_COPY_ALL_METADATA, result_flags);

#ifdef HAVE_LINK
	{
		copyfile_progress_t progress;

		progress.hardlink.target = source;

		if (callback && callback(COPYFILE_NO_ERROR, COPYFILE_HARDLINK,
					progress, callback_data, 0))
			return COPYFILE_ABORTED;

		if (unlink(dest) && errno != ENOENT)
			return COPYFILE_ERROR_UNLINK_DEST;

		while (1)
		{
			if (!link(source, dest))
			{
				if (result_flags)
					*result_flags = COPYFILE_COPY_ALL_METADATA;

				if (callback && callback(COPYFILE_EOF, COPYFILE_HARDLINK,
							progress, callback_data, 0))
					return COPYFILE_ABORTED;

				return COPYFILE_NO_ERROR;
			}
			else if (callback)
			{
				if (callback(COPYFILE_ERROR_LINK, COPYFILE_HARDLINK,
							progress, callback_data,
							errno != EXDEV && errno != EPERM))
					return COPYFILE_ERROR_LINK;
				else if (errno == EXDEV || errno == EPERM)
					break;
			}
			else /* default error handling */
			{
				if (errno == EXDEV || errno == EPERM)
					break;
				else
					return COPYFILE_ERROR_LINK;
			}
		}
	}
#endif

	return copyfile_archive_file(source, dest, st,
			COPYFILE_COPY_ALL_METADATA, result_flags,
			callback, callback_data);
}
