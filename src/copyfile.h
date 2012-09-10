/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#pragma once

#ifndef COPYFILE_H
#define COPYFILE_H 1

#include <sys/types.h>
#include <sys/stat.h>

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

	COPYFILE_ERROR_OPEN_SOURCE,
	COPYFILE_ERROR_OPEN_DEST,
	COPYFILE_ERROR_READ,
	COPYFILE_ERROR_WRITE,
	COPYFILE_ERROR_TRUNCATE,
	COPYFILE_ERROR_READLINK,
	COPYFILE_ERROR_SYMLINK,
	COPYFILE_ERROR_MALLOC,
	COPYFILE_ERROR_INTERNAL,
	COPYFILE_ERROR_STAT,
	COPYFILE_ERROR_MKDIR,
	COPYFILE_ERROR_MKFIFO,
	COPYFILE_ERROR_MKNOD,
	COPYFILE_ERROR_SOCKET,
	COPYFILE_ERROR_BIND,

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

/**
 * Copy the contents of a regular file onto a new file.
 *
 * Similarly to 'cp', this function takes no special care of the file
 * type. It just reads source until EOF, and writes the data to dest
 * (replacing it if it exists). If the input file is a pipe, it will
 * just copy the data written to the other end, and block until it is
 * closed.
 *
 * The @dest argument has to be a full path to the new file and not
 * just a directory.
 *
 * The @expected_size can hold the expected size of the file,
 * or otherwise be 0. If it's non-zero, the function will try to
 * preallocate a space for the new file.
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
copyfile_error_t copyfile_copy_regular(const char* source,
		const char* dest, off_t expected_size,
		copyfile_callback_t callback, void* callback_data);

/**
 * Copy the symlink to a new location, preserving the destination.
 *
 * Note that the destination will be preserved without modifications.
 * Especially, relative symlinks will now point relative to the new
 * location.
 *
 * The @dest argument has to be a full path to the new file and not
 * just a directory.
 *
 * If the length of symlink is known, it should be passed
 * as @expected_length. Otherwise, @expected_length should be 0.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_copy_symlink(const char* source,
		const char* dest, size_t expected_length);

/**
 * Copy the given file to a new location, preserving its type.
 *
 * If @source is a regular file, the file contents will be copied
 * to the new location. If it is a directory, an empty directory will be
 * created. If it is a symbolic link, the symbolic link will be copied.
 * Otherwise, a new special file of an appropriate type will be created.
 *
 * This is roughly equivalent to 'cp -R' without copying recursively
 * and without replacing directories with files. Note that any other
 * file type may be removed before copying.
 *
 * The @dest argument has to be a full path to the new file and not
 * just a directory.
 *
 * If the information about the file has been obtained using the lstat()
 * function, a pointer to the obtained structure should be passed
 * as @st. Otherwise, a NULL pointer should be passed.
 *
 * Please note that if the information was obtained using the stat()
 * function instead and if @source is a symbolic link, the underlying
 * file will be copied instead.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_copy_file(const char* source,
		const char* dest, const struct stat* st,
		copyfile_callback_t callback, void* callback_data);

#endif /*COPYFILE_H*/
