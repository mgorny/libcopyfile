/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#ifdef HAVE_LIBATTR
#	include <stdlib.h>
#	include <string.h>
#	include <errno.h>

#	include <attr/xattr.h>
#endif

copyfile_error_t copyfile_copy_xattr(const char* source,
		const char* dest, const struct stat* st)
{
#ifdef HAVE_LIBATTR
	/* sadly, we can't use attr_copy_file() because it doesn't provide
	 * any good way to distinguish between read and write errors. */
	{
		char list_buf[COPYFILE_BUFFER_SIZE / 2];
		char data_buf[COPYFILE_BUFFER_SIZE / 2];
		const ssize_t initial_buf_size = sizeof(list_buf);

		char* list_bufp = list_buf;
		ssize_t list_buf_size = initial_buf_size;

		char* data_bufp = data_buf;
		ssize_t data_buf_size = initial_buf_size;

		ssize_t list_len;

		const char* n;

		copyfile_error_t ret = COPYFILE_NO_ERROR;
		int saved_errno;

		list_len = llistxattr(source, 0, 0);
		if (list_len == -1)
		{
			/* if source fs doesn't support them, it doesn't have them. */
			if (errno == ENOTSUP)
				return COPYFILE_NO_ERROR;
			else
				return COPYFILE_ERROR_XATTR_LIST;
		}

		if (list_len > list_buf_size)
		{
			list_bufp = malloc(list_len);
			if (!list_bufp)
				return COPYFILE_ERROR_MALLOC;
			list_buf_size = list_len;
		}

		list_len = llistxattr(source, list_bufp, list_buf_size);
		if (list_len == -1)
		{
			if (list_buf_size > initial_buf_size)
				free(list_bufp);

			return COPYFILE_ERROR_XATTR_LIST;
		}

		for (n = list_bufp; n < &list_bufp[list_len]; n = strchr(n, 0) + 1)
		{
			ssize_t data_len;

			if (strcmp(n, "user.") && strcmp(n, "trusted."))
				continue;

			data_len = lgetxattr(source, n, 0, 0);
			if (data_len == -1)
			{
				/* return the first error
				 * but try to copy the remaining attributes first,
				 * in case user ignored errors */
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_GET;
					saved_errno = errno;
				}
				continue;
			}

			if (data_len > data_buf_size)
			{
				char* new_data_bufp = realloc(data_bufp, data_len);
				if (!new_data_bufp)
				{
					if (!ret)
					{
						ret = COPYFILE_ERROR_MALLOC;
						saved_errno = errno;
					}
					continue;
				}
				data_bufp = new_data_bufp;
				data_buf_size = data_len;
			}

			data_len = lgetxattr(source, n, data_bufp, data_buf_size);
			if (data_len == -1)
			{
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_GET;
					saved_errno = errno;
				}
				continue;
			}

			if (lsetxattr(dest, n, data_bufp, data_len, 0))
			{
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_SET;
					saved_errno = errno;
				}

				/* further tries with same attr type will fail as well */
				if (errno == ENOTSUP)
					break;
			}
		}

		if (list_buf_size > initial_buf_size)
			free(list_bufp);
		if (data_buf_size > initial_buf_size)
			free(data_bufp);

		/* set COPYFILE_COPY_XATTR even if there were no regular xattrs,
		 * failed_flags will unset it on failure */
		if (ret)
			errno = saved_errno;
		return ret;
	}
#endif /*HAVE_LIBATTR*/

	return COPYFILE_ERROR_UNSUPPORTED;
}
