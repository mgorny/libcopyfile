/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#ifdef HAVE_LIBACL
#	include <sys/acl.h>
#	include <sys/stat.h>
#	include <errno.h>

static const acl_type_t acl_types[] =
{
	ACL_TYPE_ACCESS,
	ACL_TYPE_DEFAULT
};
#endif /*HAVE_LIBACL*/

copyfile_error_t copyfile_copy_acl(const char* source,
		const char* dest, const struct stat* st)
{
#ifdef HAVE_LIBACL
	{
		copyfile_error_t ret = COPYFILE_NO_ERROR;
		int saved_errno;

		int i;

#ifndef HAVE_ACL_GET_LINK_NP
		{
			struct stat buf;

			if (!st)
			{
				if (lstat(source, &buf))
					return COPYFILE_ERROR_STAT;

				st = &buf;
			}

			if (S_ISLNK(st->st_mode))
				return COPYFILE_NO_ERROR;
		}
#endif /*!HAVE_ACL_GET_LINK_NP*/

		for (i = 0; i < 2; ++i)
		{
			acl_t acl;

#ifdef HAVE_ACL_GET_LINK_NP
			acl = acl_get_link_np(source, acl_types[i]);
#else
			acl = acl_get_file(source, acl_types[i]);
#endif
			if (acl)
			{
				int aclret;

#ifdef HAVE_ACL_GET_LINK_NP
				aclret = acl_set_link_np(dest, acl_types[i], acl);
#else
				aclret = acl_set_file(dest, acl_types[i], acl);
#endif
				if (aclret && !ret)
				{
					ret = COPYFILE_ERROR_ACL_SET;
					saved_errno = errno;
				}

				acl_free(acl);
			}
			else
			{
				/* ACLs not supported? fine, nothing to copy. */
				if (errno == EOPNOTSUPP)
					return COPYFILE_NO_ERROR;
				else if (i > 0 && errno == EACCES)
					/* ACL_TYPE_DEFAULT on non-dir, likely */;
				else if (!ret)
				{
					ret = COPYFILE_ERROR_ACL_GET;
					saved_errno = errno;
				}
			}
		}

		if (ret)
			errno = saved_errno;
		return ret;
	}
#endif /*HAVE_LIBACL*/

	return COPYFILE_ERROR_UNSUPPORTED;
}
