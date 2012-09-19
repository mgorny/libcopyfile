/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#include <sys/stat.h>

copyfile_error_t copyfile_archive_file(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags,
		copyfile_callback_t callback, void* callback_data)
{
	struct stat buf;
	copyfile_error_t ret;

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

	ret = copyfile_copy_file(source, dest, st, callback, callback_data);
	if (ret)
	{
		if (result_flags)
			*result_flags = 0;
		return ret;
	}

	return copyfile_copy_metadata(source, dest, st, flags, result_flags);
}
