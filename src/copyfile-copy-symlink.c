/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"
#include "common.h"

#ifdef S_IFLNK

#	include <sys/stat.h>
#	include <sys/types.h>
#	include <limits.h>
#	include <stdlib.h>
#	include <unistd.h>

static copyfile_error_t try_copy_symlink(const char* source,
		const char* dest, char* buf, ssize_t buf_size)
{
	ssize_t rd = readlink(source, buf, buf_size);

	if (rd == -1)
		return COPYFILE_ERROR_READLINK;

	if (rd < buf_size)
	{
		buf[rd + 1] = 0;

		if (symlink(buf, dest))
			return COPYFILE_ERROR_SYMLINK;
		return COPYFILE_NO_ERROR;
	}

	return COPYFILE_EOF;
}
#endif /*S_IFLNK*/

copyfile_error_t copyfile_copy_symlink(const char* source,
		const char* dest, size_t expected_length,
		copyfile_callback_t callback, void* callback_data)
{
#ifdef S_IFLNK
	/* remember that the last cell is for null-terminator */

	/* try to avoid dynamic allocation */
	char buf[COPYFILE_BUFFER_SIZE];
	size_t buf_size = sizeof(buf);

	copyfile_progress_t progress;

	progress.symlink.length = expected_length;
	if (callback && callback(COPYFILE_NO_ERROR, COPYFILE_SYMLINK,
				progress, callback_data))
		return COPYFILE_ABORTED;

	if (expected_length < buf_size)
	{
		int ret;

		while (1)
		{
			ret = try_copy_symlink(source, dest, buf, buf_size);

			if (ret != COPYFILE_EOF)
			{
				if (!ret)
				{
					if (callback)
					{
						progress.symlink.target = buf;
						if (callback(COPYFILE_EOF, COPYFILE_SYMLINK,
									progress, callback_data))
							return COPYFILE_ABORTED;
					}

					return ret;
				}

				/* does the user want to retry? */
				if (!callback || callback(ret, COPYFILE_SYMLINK,
							progress, callback_data))
					return ret;
			}
			else
				break;
		}

		buf_size *= 2;
	}
	else
		buf_size = expected_length + 1;

	{
		char* buf = 0;
		int ret;

		/* size_t is always unsigned, so -1 safely becomes SIZE_MAX */
		const size_t my_size_max = -1;
		const size_t max_size = my_size_max > SSIZE_MAX
			? SSIZE_MAX : my_size_max;

		while (1)
		{
			char* next_buf = realloc(buf, buf_size);

			if (!next_buf)
			{
				ret = COPYFILE_ERROR_MALLOC;

				/* retry? */
				if (callback && !callback(ret, COPYFILE_SYMLINK,
							progress, callback_data))
					continue;

				if (!buf) /* avoid freeing in the less likely branch */
					return ret;
				break;
			}
			buf = next_buf;

			ret = try_copy_symlink(source, dest, buf, buf_size);

			if (!ret)
			{
				if (callback)
				{
					progress.symlink.target = buf;
					if (callback(COPYFILE_EOF, COPYFILE_SYMLINK,
								progress, callback_data))
						return COPYFILE_ABORTED;
				}
				break;
			}
			else if (ret == COPYFILE_EOF)
			{
				if (buf_size < max_size / 2)
					buf_size *= 2;
				else
				{
					if (buf_size != max_size)
						buf_size = max_size;
					else
					{
						ret = COPYFILE_ERROR_SYMLINK_TARGET_TOO_LONG;
						break;
					}
				}
			}
			else
			{
				/* does the user want to retry? */
				if (!callback && callback(ret, COPYFILE_SYMLINK,
							progress, callback_data))
					break;
			}
		}

		free(buf);
		return ret;
	}
#endif /*S_IFLNK*/

	return COPYFILE_ERROR_UNSUPPORTED;
}
