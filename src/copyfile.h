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
	/**
	 * A special value indicating complete success. It is guaranteed to
	 * evaluate to zero, so you can use the '!' operator to check
	 * function return for success.
	 *
	 * The value of errno is undefined.
	 */
	COPYFILE_NO_ERROR = 0,

	/**
	 * Common error domains. These values indicate the function which
	 * returned an error. The system error code can be found in errno.
	 */
	COPYFILE_ERROR_OPEN_SOURCE,
	COPYFILE_ERROR_OPEN_DEST,
	COPYFILE_ERROR_READ,
	COPYFILE_ERROR_WRITE,
	COPYFILE_ERROR_TRUNCATE,
	COPYFILE_ERROR_READLINK,
	COPYFILE_ERROR_SYMLINK,
	COPYFILE_ERROR_MALLOC,
	COPYFILE_ERROR_STAT,
	COPYFILE_ERROR_MKDIR,
	COPYFILE_ERROR_MKFIFO,
	COPYFILE_ERROR_MKNOD,
	COPYFILE_ERROR_SOCKET,
	COPYFILE_ERROR_BIND,
	COPYFILE_ERROR_CHOWN,
	COPYFILE_ERROR_CHMOD,
	COPYFILE_ERROR_UTIME,
	COPYFILE_ERROR_XATTR_LIST,
	COPYFILE_ERROR_XATTR_GET,
	COPYFILE_ERROR_XATTR_SET,
	COPYFILE_ERROR_DOMAIN_MAX,

	/**
	 * An internal error. This should never happen, and is always backed
	 * up with an assert(). If it happens, you should report a bug.
	 */
	COPYFILE_ERROR_INTERNAL = 100,
	/**
	 * The symlink target is longer than we can actually handle. This is
	 * something you should probably investigate and report, since this
	 * means that the readlink() function probably can't read it...
	 * or at least can't fit the length in the return type.
	 */
	COPYFILE_ERROR_SYMLINK_TARGET_TOO_LONG,
	/**
	 * The destination for the unix socket is longer than sockaddr_un
	 * can handle. There is probably nothing we can do about it.
	 */
	COPYFILE_ERROR_SOCKET_DEST_TOO_LONG,
	/**
	 * A particular feature is unsupported or the support is disabled.
	 */
	COPYFILE_ERROR_UNSUPPORTED,

	/**
	 * The operation was aborted by a callback function.
	 */
	COPYFILE_ABORTED = 200,
	/**
	 * A special end-of-file status constant for callback.
	 */
	COPYFILE_EOF,

	COPYFILE_ERROR_MAX
} copyfile_error_t;

/**
 * Constants for metadata copying flags.
 */
typedef enum
{
	/**
	 * Copy the user owner of the file.
	 */
	COPYFILE_COPY_USER = 0x01,
	/**
	 * Copy the group owner the file.
	 */
	COPYFILE_COPY_GROUP = 0x02,
	/**
	 * Copy both the user and the group owner of the file.
	 */
	COPYFILE_COPY_OWNER = COPYFILE_COPY_USER | COPYFILE_COPY_GROUP,

	/**
	 * Copy mode (permissions + SUID/SGID/VTX bits) to the file.
	 *
	 * Note that if this is not used but owner of the file is changed,
	 * the resulting mode may be affected by the call to chown().
	 */
	COPYFILE_COPY_MODE = 0x04,

	/**
	 * Copy file modification time.
	 *
	 * Note that on some systems it is impossible to change mtime
	 * without changing atime. On those systems, this will be equivalent
	 * to COPYFILE_COPY_TIMES.
	 */
	COPYFILE_COPY_MTIME = 0x08,
	/**
	 * Copy file access time.
	 *
	 * Note that on some systems it is impossible to change atime
	 * without changing mtime. On those systems, this will be equivalent
	 * to COPYFILE_COPY_TIMES.
	 */
	COPYFILE_COPY_ATIME = 0x10,
	/**
	 * Copy both file access & modification times.
	 */
	COPYFILE_COPY_TIMES = COPYFILE_COPY_MTIME | COPYFILE_COPY_ATIME,

	/**
	 * Copy all supported stat() metadata of a file.
	 */
	COPYFILE_COPY_STAT = COPYFILE_COPY_OWNER | COPYFILE_COPY_MODE
		| COPYFILE_COPY_TIMES,

	/**
	 * Mask of the valid values.
	 */
	COPYFILE_MASK = 0x1f
} copyfile_metadata_flag_t;

/**
 * An uniform type for callback progress information.
 */
typedef union
{
	/**
	 * Progress information for regular file (stream) copy.
	 */
	struct
	{
		/**
		 * Current offset in the stream.
		 *
		 * The offset will be calculated from the beginning of copying.
		 * In an EOF case, it will be the actual amount of data copied.
		 */
		off_t offset;
		/**
		 * Apparent file size.
		 *
		 * The size will be the value passed by user or otherwise
		 * obtained from stat() if relevant. It may be 0 if not
		 * provided. It won't be updated in an EOF case, thus it may be
		 * smaller than offset.
		 */
		off_t size;
	} data;

	/**
	 * Symbolic link copying-specific information.
	 */
	union
	{
		/**
		 * Symlink target length, in case of symbolic link copy.
		 *
		 * This is available only in a non-EOF callback. It is a value
		 * passed by user or otherwise obtained from lstat() if relevant.
		 * It may be 0 if not provided. It may be outdated.
		 */
		off_t length;
		/**
		 * Symlink target, in case of symbolic link copy.
		 *
		 * This is available only in an EOF callback.
		 */
		const char* target;
	} symlink;

	/**
	 * Device identifier, in case of device file copy.
	 *
	 * This is available in both EOF and non-EOF callbacks.
	 */
	dev_t device;
} copyfile_progress_t;

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
 * In any of those cases, the @type parameter will hold the file type,
 * in form of a mode_t constant (as returned by stat()). The @progress
 * parameter carries detailed progress information, and @data carries
 * any user-provided data.
 *
 * At the start of copying and during the process, the callback is
 * called with @state == COPYFILE_NO_ERROR. At the end of file, @state
 * is COPYFILE_EOF.
 *
 * In case of an error, @state has any other value; and the system errno
 * variable is set appropriately. Note that the errno may be EINTR as
 * well, which allows the user to interrupt the copy and enter
 * the callback.
 *
 * The data types available in the @progress union for various file
 * types and states are described in copyfile_progress_t description.
 * In other cases, the contents of the union are undefined.
 *
 * In normal call case, the callback should return 0 in order to
 * continue copying, or a non-zero value to abort it. In the latter
 * case, the function will return with COPYFILE_ABORTED.
 *
 * Please note that in case of an error, callback should return 0
 * in order to retry the operation, and a non-zero value to fail.
 * In that case, the original failure will be returned by function.
 *
 * Also note that the functions don't copy errno before calling
 * the callback on error. If your callback function calls functions
 * which can change errno in that case, you need to either restore its
 * value in the callback or be aware that the errno after the function
 * return will be set in callback.
 */
typedef int (*copyfile_callback_t)(copyfile_error_t state, mode_t type,
		copyfile_progress_t progress, void* data);

/**
 * Copy the contents of an input stream onto an output stream.
 *
 * This function takes no special care of the file type -- it just reads
 * the input stream until it reaches EOF, and writes to the output
 * stream.
 *
 * The streams will not be closed. In case of an error, the current
 * offset on both streams is undefined.
 *
 * The @expected_size can hold the expected size of the file,
 * or otherwise be 0. It will be only passed through to the callback.
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
		off_t expected_size, copyfile_callback_t callback,
		void* callback_data);

/**
 * Copy the contents of a regular file onto a new file.
 *
 * Similarly to 'cp', this function takes no special care of the file
 * type. It just reads @source until EOF, and writes the data to @dest.
 * If @dest exists and is a regular file, it will be replaced. If it is
 * a link to regular file (either hard or symbolic one), the target file
 * will be replaced. It is a named pipe or a special file, the data will
 * be copied into it.
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
 * just a directory. It must not exist.
 *
 * If the length of symlink is known, it should be passed
 * as @expected_length. Otherwise, @expected_length should be 0.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_copy_symlink(const char* source,
		const char* dest, size_t expected_length,
		copyfile_callback_t callback, void* callback_data);

/**
 * Create a special (incopiable) file.
 *
 * A new file of type @ftype (stat() 'st_mode & S_IFMT' format)
 * at location @path. If @path exists already, the call will fail.
 *
 * If @ftype is S_IFCHR or S_IFBLK (character or block device),
 * the @devid parameter should contain the device number. Otherwise, it
 * can contain any value and will be ignored.
 *
 * If @callback is non-NULL, it will be used to report progress and/or
 * errors. The @callback_data will be passed to it. For more details,
 * see copyfile_callback_t description.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_create_special(const char* path, mode_t ftype,
		dev_t devid, copyfile_callback_t callback, void* callback_data);

/**
 * Copy the given file to a new location, preserving its type.
 *
 * If @source is a regular file, the file contents will be copied
 * to the new location. If it is a directory, an empty directory will be
 * created. If it is a symbolic link, the symbolic link will be copied.
 * Otherwise, a new special file of an appropriate type will be created.
 *
 * This is roughly equivalent to 'cp -R' without copying recursively
 * and without any special replacement behavior.
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
copyfile_error_t copyfile_copy_file(const char* source,
		const char* dest, const struct stat* st,
		copyfile_callback_t callback, void* callback_data);

/**
 * Set stat() metadata for a given file.
 *
 * The new metadata should be passed as @st. It must not be NULL. This
 * function assumes that the metadata is exactly fit for the new file,
 * including the file type. If @dest is of other type, incorrect
 * behavior may occur (e.g. if @dest is a symbolic link and @st contains
 * information about a regular file, the symbolic link target
 * permissions may be modified instead).
 *
 * The @flags parameter specifies which properties are to be modified.
 * In order to copy all the stat() metadata, pass COPYFILE_COPY_STAT.
 * For more fine-grained control, see the description
 * of copyfile_metadata_flag_t.
 *
 * If @result_flags is not NULL, the bit-field pointed by it will
 * contain a copy of flags explaining which operations were done
 * successfully (it will be reset to zero first). It will be equal
 * to @flags if all changes were done successfully. If the operation is
 * aborted due to an error, it will state which changes were done
 * before the error. If some of the changes were unsupported,
 * the relevant flags will be unset.
 *
 * This function does not provide a fine-grained error reporting.
 * On the first major failure, it will abort, possibly having modified
 * the metadata partially. Unsupported operation errors will be ignored.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_set_stat(const char* path,
		const struct stat* st, unsigned int flags,
		unsigned int* result_flags);

/**
 * Copy extended attributes of a file.
 *
 * If extended attribute support is disabled, it will return
 * COPYFILE_ERROR_UNSUPPORTED. If source file does not support extended
 * attributes, it will return COPYFILE_NO_ERROR (since there's nothing
 * to copy).
 *
 * Otherwise, it will try hard to copy all the extended attributes.
 * The call will return COPYFILE_NO_ERROR if all attributes were copied
 * correctly, or the first error which occurs. Note that in case
 * of an error other than ENOTSUP (xattr unsupported on destination
 * filesystem), it will still try to copy the remaining attributes.
 *
 * Returns 0 on success, an error otherwise. errno will hold the system
 * error code.
 */
copyfile_error_t copyfile_copy_xattr(const char* source,
		const char* dest);

#endif /*COPYFILE_H*/
