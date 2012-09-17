/* libcopyfile -- common constants
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#pragma once

#ifndef COPYFILE_COMMON_H
#define COPYFILE_COMMON_H 1

#include <sys/stat.h>

#ifndef COPYFILE_BUFFER_SIZE
#	define COPYFILE_BUFFER_SIZE 4096
#endif

#ifndef COPYFILE_CALLBACK_OPCOUNT
#	define COPYFILE_CALLBACK_OPCOUNT 64
#endif

static const int not_reached = 0;

/* default permissions */
enum modes
{
	perm_dir = S_IRWXU
#ifdef S_IRWXG
		| S_IRWXG
#endif
#ifdef S_IRWXO
		| S_IRWXO
#endif
		,
	perm_file = perm_dir & ~(S_IXUSR
#ifdef S_IXGRP
			| S_IXGRP
#endif
#ifdef S_IXOTH
			| S_IXOTH
#endif
			),
	all_perm_bits = perm_dir
#ifdef S_ISUID
		| S_ISUID
#endif
#ifdef S_ISGID
		| S_ISGID
#endif
#ifdef S_ISVTX
		| S_ISVTX
#endif
};

#endif /*COPYFILE_COMMON_H*/
