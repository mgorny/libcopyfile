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

copyfile_error_t copyfile_copy_metadata(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags)
{
	struct stat buf;
	unsigned int done = 0;

	if (!flags)
		flags = COPYFILE_COPY_ALL_METADATA;
	if (result_flags)
		*result_flags = 0;
	if (!st)
	{
		if (lstat(source, &buf))
			return COPYFILE_ERROR_STAT;

		st = &buf;
	}

	/* Now, order is important.
	 *
	 * 1) chown() because changing owners can change modes,
	 * 2) xattr because xattr can sometimes copy ACLs as well,
	 * 3) ACLs because copying ACLs implies chmod(),
	 * 4) remaining stat() stuff (including chmod() for special modes).
	 */

	if (flags & COPYFILE_COPY_OWNER)
	{
		copyfile_error_t ret = copyfile_set_stat(dest, st,
				flags & COPYFILE_COPY_OWNER, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;

		if (ret)
			return ret;
	}

	if (flags & COPYFILE_COPY_XATTR_ALL)
	{
		copyfile_copy_xattr(source, dest, flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_CAP)
	{
		copyfile_copy_cap(source, dest, st, flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_ACL)
	{
		copyfile_copy_acl(source, dest, st, flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_STAT)
	{
		copyfile_error_t ret = copyfile_set_stat(dest, st,
				flags, &done);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;

		if (ret)
			return ret;
	}

	return COPYFILE_NO_ERROR;
}
