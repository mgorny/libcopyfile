/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "libcopyfile.h"
#include "common.h"

#ifdef HAVE_BTRFS_IOCTL_H
#	include <sys/ioctl.h>
#	include <btrfs/ioctl.h>
#endif

copyfile_error_t copyfile_clone_stream(int fd_in, int fd_out)
{
	/* btrfs? */
#ifdef HAVE_BTRFS_IOCTL_H
	if (!ioctl(fd_out, BTRFS_IOC_CLONE, fd_in))
		return COPYFILE_NO_ERROR;

	return COPYFILE_ERROR_IOCTL_CLONE;
#endif

	return COPYFILE_ERROR_UNSUPPORTED;
}
