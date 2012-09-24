/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
#include "common.h"

#include <sys/stat.h>
#include <errno.h>

copyfile_error_t copyfile_copy_metadata(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags)
{
	struct stat buf;

	copyfile_error_t ret = COPYFILE_NO_ERROR;
	int saved_errno;

	if (!flags)
		flags = COPYFILE_COPY_ALL_METADATA;
	if (result_flags)
		*result_flags = 0;
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

	if (flags & COPYFILE_COPY_XATTR)
	{
		copyfile_error_t lret = copyfile_copy_xattr(source, dest, st);

		if (!lret)
		{
			if (result_flags)
				*result_flags |= COPYFILE_COPY_XATTR;
		}
		else if (lret == COPYFILE_ERROR_XATTR_GET && !ret)
		{
			ret = lret;
			saved_errno = errno;
		}
	}

	if (flags & COPYFILE_COPY_CAP)
	{
		copyfile_error_t lret = copyfile_copy_cap(source, dest, st);

		if (!lret)
		{
			if (result_flags)
				*result_flags |= COPYFILE_COPY_CAP;
		}
		else if (lret == COPYFILE_ERROR_CAP_GET && !ret)
		{
			ret = lret;
			saved_errno = errno;
		}
	}

	if (flags & COPYFILE_COPY_ACL)
	{
		copyfile_error_t lret = copyfile_copy_acl(source, dest, st);

		if (!lret)
		{
			if (result_flags)
				*result_flags |= COPYFILE_COPY_ACL;
		}
		else if (lret == COPYFILE_ERROR_ACL_GET && !ret)
		{
			ret = lret;
			saved_errno = errno;
		}
	}

	if (flags & COPYFILE_COPY_STAT)
	{
		unsigned int done = copyfile_set_stat(dest, st, flags);

		if (result_flags)
			*result_flags |= done;
	}

	if (ret)
		errno = saved_errno;
	return ret;
}
