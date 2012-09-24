/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
#include "common.h"

#ifdef HAVE_LIBCAP
#	include <sys/capability.h>
#	include <sys/stat.h>
#	include <errno.h>
#endif /*HAVE_LIBCAP*/

copyfile_error_t copyfile_copy_cap(const char* source,
		const char* dest, const struct stat* st)
{
#ifdef HAVE_LIBCAP
	{
		cap_t cap = 0;

		{
			struct stat buf;

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

			if (!S_ISREG(st->st_mode))
				return COPYFILE_NO_ERROR;
		}

		/* ENODATA - empty caps
		 * ENOTSUP - caps not supported */
		cap = cap_get_file(source);
		if (!cap && errno != ENODATA)
		{
			if (errno == ENOTSUP)
				return COPYFILE_NO_ERROR;
			else
				return COPYFILE_ERROR_CAP_GET;
		}

		/* ENODATA - empty->empty... */
		if (cap_set_file(dest, cap) && errno != ENODATA)
		{
			cap_free(cap);
			return COPYFILE_ERROR_CAP_SET;
		}

		cap_free(cap);
		return COPYFILE_NO_ERROR;
	}
#endif /*HAVE_LIBCAP*/

	return COPYFILE_ERROR_UNSUPPORTED;
}

