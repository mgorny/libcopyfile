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
#ifdef S_IFLNK
		case S_IFLNK:
			return copyfile_copy_symlink(source, dest, st->st_size,
					callback, callback_data);
#endif /*S_IFLNK*/
		default:
			return copyfile_create_special(dest, ftype, st->st_rdev,
					callback, callback_data);
	}
}
