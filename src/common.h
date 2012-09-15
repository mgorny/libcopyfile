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
	perm_dir = S_IRWXU | S_IRWXG | S_IRWXO,
	perm_file = perm_dir & ~(S_IXUSR | S_IXGRP | S_IXOTH),
	all_perm_bits = perm_dir | S_ISUID | S_ISGID | S_ISVTX
};

#endif /*COPYFILE_COMMON_H*/
