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
#include <errno.h>

copyfile_error_t copyfile_copy_metadata(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags)
{
	struct stat buf;
	unsigned int done = 0;

	copyfile_error_t ret = COPYFILE_NO_ERROR;
	int saved_errno;

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
		unsigned int done = copyfile_set_stat(dest, st,
				flags & COPYFILE_COPY_OWNER);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_XATTR_ALL)
	{
		if (copyfile_copy_xattr(source, dest, flags, &done)
				== COPYFILE_ERROR_XATTR_GET && !ret)
		{
			ret = COPYFILE_ERROR_XATTR_GET;
			saved_errno = errno;
		}

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_CAP)
	{
		if (copyfile_copy_cap(source, dest, st, flags, &done)
				== COPYFILE_ERROR_CAP_GET && !ret)
		{
			ret = COPYFILE_ERROR_CAP_GET;
			saved_errno = errno;
		}

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_ACL)
	{
		if (copyfile_copy_acl(source, dest, st, flags, &done)
				== COPYFILE_ERROR_ACL_GET && !ret)
		{
			ret = COPYFILE_ERROR_ACL_GET;
			saved_errno = errno;
		}

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (flags & COPYFILE_COPY_STAT)
	{
		unsigned int done = copyfile_set_stat(dest, st, flags);

		flags &= ~done;
		if (result_flags)
			*result_flags |= done;
	}

	if (ret)
		errno = saved_errno;
	return ret;
}
