/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"

const char* copyfile_error_message(copyfile_error_t err)
{
	const char* ret = "Unknown/invalid copyfile error";

	switch (err)
	{
		case COPYFILE_NO_ERROR:
			ret = "The operation completed successfully";
			break;
		case COPYFILE_ERROR_OPEN_SOURCE:
			ret = "Unable to open the source file";
			break;
		case COPYFILE_ERROR_OPEN_DEST:
			ret = "Unable to open the destination file";
			break;
		case COPYFILE_ERROR_READ:
			ret = "Unable to read the source file";
			break;
		case COPYFILE_ERROR_WRITE:
			ret = "Unable to write the destination file";
			break;
		case COPYFILE_ERROR_TRUNCATE:
			ret = "Unable to truncate the destination file";
			break;
		case COPYFILE_ERROR_READLINK:
			ret = "Unable to read the symbolic link target";
			break;
		case COPYFILE_ERROR_SYMLINK:
			ret = "Unable to create a symbolic link";
			break;
		case COPYFILE_ERROR_MALLOC:
			ret = "Unable to allocate memory";
			break;
		case COPYFILE_ERROR_STAT:
			ret = "Unable to obtain the stat() information for the source file";
			break;
		case COPYFILE_ERROR_MKDIR:
			ret = "Unable to create a directory";
			break;
		case COPYFILE_ERROR_MKFIFO:
			ret = "Unable to create a named pipe";
			break;
		case COPYFILE_ERROR_MKNOD:
			ret = "Unable to create a special file";
			break;
		case COPYFILE_ERROR_SOCKET:
			ret = "Unable to create a UNIX socket";
			break;
		case COPYFILE_ERROR_BIND:
			ret = "Unable to bind a UNIX socket";
			break;
		case COPYFILE_ERROR_XATTR_LIST:
			ret = "Unable to list the extended attributes";
			break;
		case COPYFILE_ERROR_XATTR_GET:
			ret = "Unable to get the value of an extended attribute";
			break;
		case COPYFILE_ERROR_XATTR_SET:
			ret = "Unable to set the value of an extended attribute";
			break;
		case COPYFILE_ERROR_ACL_GET:
			ret = "Unable to get an ACL for the source file";
			break;
		case COPYFILE_ERROR_ACL_SET:
			ret = "Unable to set an ACL for the destination file";
			break;
		case COPYFILE_ERROR_CAP_GET:
			ret = "Unable to get the capabilities of the source file";
			break;
		case COPYFILE_ERROR_CAP_SET:
			ret = "Unable to set the capabilities of the destination file";
			break;
		case COPYFILE_ERROR_LINK:
			ret = "Unable to create a hard link";
			break;
		case COPYFILE_ERROR_RENAME:
			ret = "Unable to atomically rename (move) the file";
			break;
		case COPYFILE_ERROR_UNLINK_SOURCE:
			ret = "Unable to unlink source file (after moving)";
			break;
		case COPYFILE_ERROR_UNLINK_DEST:
			ret = "Unable to unlink destination file (before replacing)";
			break;

		case COPYFILE_ERROR_INTERNAL:
			ret = "Internal libcopyfile error (please report!)";
			break;
		case COPYFILE_ERROR_SYMLINK_TARGET_TOO_LONG:
			ret = "The symbolic link target is too long for the platform (please report!)";
			break;
		case COPYFILE_ERROR_SOCKET_DEST_TOO_LONG:
			ret = "The UNIX socket name is too long for the platform (please report!)";
			break;
		case COPYFILE_ERROR_UNSUPPORTED:
			ret = "The requested operation is not supported by the platform";
			break;
		case COPYFILE_ABORTED:
			ret = "The requested operation has been aborted by the user";
			break;

		/* do not put default: here, so the compiler will complain about
		 * missing cases. */
		case COPYFILE_ERROR_DOMAIN_MAX:
		case COPYFILE_EOF:
		case COPYFILE_ERROR_MAX:
			break;
	}

	return ret;
}
