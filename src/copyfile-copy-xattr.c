/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#ifdef HAVE_XATTR
#	include <stdlib.h>
#	include <string.h>
#	include <errno.h>

#	ifdef HAVE_LGETXATTR /* GNU/Linux */
#		include <attr/xattr.h>
#	endif
#	ifdef HAVE_EXTATTR_GET_LINK /* BSD */
#		include <sys/extattr.h>
#	endif
#endif

copyfile_error_t copyfile_copy_xattr(const char* source,
		const char* dest, const struct stat* st)
{
#ifdef HAVE_XATTR
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

		char* n;

		copyfile_error_t ret = COPYFILE_NO_ERROR;
		int saved_errno;

#ifdef HAVE_EXTATTR_GET_LINK
		unsigned char next_len;
#endif

#ifdef HAVE_LGETXATTR
		list_len = llistxattr(source, 0, 0);
#endif
#ifdef HAVE_EXTATTR_GET_LINK
		list_len = extattr_list_link(source, EXTATTR_NAMESPACE_USER,
				0, 0);
#endif
		if (list_len == -1)
		{
			/* if source fs doesn't support them, it doesn't have them. */
			if (errno == EOPNOTSUPP)
				return COPYFILE_NO_ERROR;
			else
				return COPYFILE_ERROR_XATTR_LIST;
		}

		if (list_len > list_buf_size)
		{
			/* On BSD, the list is not null-terminated,
			 * so we need one more cell for NULL. */
			list_bufp = malloc(list_len + 1);
			if (!list_bufp)
				return COPYFILE_ERROR_MALLOC;
			list_buf_size = list_len;
		}

#ifdef HAVE_LGETXATTR
		list_len = llistxattr(source, list_bufp, list_buf_size);
#endif
#ifdef HAVE_EXTATTR_GET_LINK
		list_len = extattr_list_link(source, EXTATTR_NAMESPACE_USER,
				list_bufp, list_buf_size);
#endif
		if (list_len == -1)
		{
			if (list_buf_size > initial_buf_size)
				free(list_bufp);

			return COPYFILE_ERROR_XATTR_LIST;
		}

		n = list_bufp;
#ifdef HAVE_EXTATTR_GET_LINK
		/* This is safe since buffer will always have at least a few
		 * bytes. */
		next_len = *n++;
#endif

		for (; n < &list_bufp[list_len]; n = strchr(n, 0) + 1)
		{
			ssize_t data_len;

#ifdef HAVE_EXTATTR_GET_LINK
			/* On BSD, the list consists of pascal strings...
			 * let's null-terminate it. */
			unsigned int next_len_tmp = n[next_len];
			n[next_len] = 0; /* null-terminate */
			next_len = next_len_tmp;
#endif

#ifdef HAVE_LGETXATTR
			/* On Linux, namespace is stored in the attribute name. */
			if (strcmp(n, "user.") && strcmp(n, "trusted."))
				continue;
#endif

#ifdef HAVE_LGETXATTR
			data_len = lgetxattr(source, n, 0, 0);
#endif
#ifdef HAVE_EXTATTR_GET_LINK
			data_len = extattr_get_link(source, EXTATTR_NAMESPACE_USER,
					n, 0, 0);
#endif
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

#ifdef HAVE_LGETXATTR
			data_len = lgetxattr(source, n, data_bufp, data_buf_size);
#endif
#ifdef HAVE_EXTATTR_GET_LINK
			data_len = extattr_get_link(source, EXTATTR_NAMESPACE_USER,
					n, data_bufp, data_buf_size);
#endif
			if (data_len == -1)
			{
				if (!ret)
				{
					ret = COPYFILE_ERROR_XATTR_GET;
					saved_errno = errno;
				}
				continue;
			}

#ifdef HAVE_LGETXATTR
			if (lsetxattr(dest, n, data_bufp, data_len, 0))
#endif
#ifdef HAVE_EXTATTR_GET_LINK
			if (extattr_set_link(dest, EXTATTR_NAMESPACE_USER, n,
					data_bufp, data_len) != data_len)
#endif
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
#endif /*HAVE_ATTR*/

	return COPYFILE_ERROR_UNSUPPORTED;
}
