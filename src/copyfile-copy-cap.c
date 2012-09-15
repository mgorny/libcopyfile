/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#ifdef HAVE_LIBCAP
#	include <sys/capability.h>
#	include <sys/stat.h>
#	include <errno.h>
#endif /*HAVE_LIBCAP*/

copyfile_error_t copyfile_copy_cap(const char* source,
		const char* dest, const struct stat* st,
		unsigned int flags, unsigned int* result_flags)
{
	if (result_flags)
		*result_flags = 0;

#ifdef HAVE_LIBCAP
	if (!flags)
		flags = COPYFILE_COPY_CAP;

	if (flags & COPYFILE_COPY_CAP)
	{
		cap_t cap = 0;

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
			return COPYFILE_ERROR_CAP_SET;

		if (result_flags)
			*result_flags |= COPYFILE_COPY_CAP;

		return COPYFILE_NO_ERROR;
	}
#endif /*HAVE_LIBCAP*/

	return COPYFILE_ERROR_UNSUPPORTED;
}

