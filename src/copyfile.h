/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#pragma once

#ifndef COPYFILE_H
#define COPYFILE_H 1

#include <sys/types.h>

/**
 * The error return type.
 *
 * It describes the source (syscall) of an error; if not
 * COPYFILE_NO_ERROR, the system errno variable contains the detailed
 * error code.
 */
typedef enum
{
	COPYFILE_NO_ERROR = 0,

	COPYFILE_ERROR_READ,
	COPYFILE_ERROR_WRITE,

	COPYFILE_ABORTED,
	COPYFILE_EOF,
	COPYFILE_ERROR_MAX
} copyfile_error_t;

/**
 * The callback for copyfile_copy_stream().
 *
 * The callback function is called in the following cases:
 * - at the start of copying,
 * - an undefined number of times during the copying (which may depend
 *   on amounts of data copied or function calls),
 * - at the end of copying,
 * - in case of an error.
 *
 * In any of those cases, the @pos parameter states the total number of
 * bytes read from the stream. The @data parameter carries any
 * user-provided data.
 *
 * At the start of copying and during the process, the callback is
 * called with @state == COPYFILE_NO_ERROR. At the end of file, @state
 * == COPYFILE_EOF.
 *
 * In case of an error, @state has any other value; and the system errno
 * variable is set appropriately. Note that the errno may be EINTR as
 * well, which allows the user to interrupt the copy and enter
 * the callback.
 *
 * In normal call case, the callback should return 0 in order to
 * continue copying, or a non-zero value to abort it. In the latter
 * case, the function will return with COPYFILE_ABORTED.
 *
 * Please note that in case of an error, callback should return 0
 * in order to retry the operation, and a non-zero value to fail.
 * In that case, the original failure will be returned by function.
 */
typedef int (*copyfile_callback_t)(copyfile_error_t state, off_t pos,
		void* data);

/**
 * Copy the contents of an input stream onto an output stream.
 *
 * This function takes no special care of the file type -- it just reads
 * the input stream until it reaches EOF, and writes to the output
 * stream. If the input stream is a pipe, it will actually block until
 * the other end is closed.
 *
 * The streams will not be closed. In case of an error, the current
 * offset on both streams is undefined.
 *
 * If @callback is non-NULL, it will be used to report progress and/or
 * errors. The @callback_data will be passed to it. For more details,
 * see copyfile_callback_t description.
 *
 * If @callback is NULL, default error handling will be used. The EINTR
 * error will be retried indefinitely, and other errors will cause
 * immediate failure.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_copy_stream(int fd_in, int fd_out,
		copyfile_callback_t callback, void* callback_data);

#endif /*COPYFILE_H*/
