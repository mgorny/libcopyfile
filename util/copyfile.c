/* libcopyfile
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "copyfile.h"

#include <stdio.h>
#include <unistd.h>

#ifdef HAVE_GETOPT_LONG
#	include <getopt.h>
#endif

static const char* const copyfile_opts = "hV";

#ifdef HAVE_GETOPT_LONG

static const struct option copyfile_long_opts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },

	{ 0, 0, 0, 0 }
};

#endif /*HAVE_GETOPT_LONG*/

static const char* const copyfile_help_f =
"Usage: %s [OPTIONS] SOURCE DEST\n"
"\n"
"Copy a single file SOURCE to a new full path DEST. DEST must not\n"
"be just a directory, it has to contain the filename as well.\n"
"\n"
"  -h, --help            print help message\n"
"  -V, --version         print program version\n";

int main(int argc, char* argv[])
{
	while (1)
	{
		int opt;

#ifdef HAVE_GETOPT_LONG
		opt = getopt_long(argc, argv, copyfile_opts,
				copyfile_long_opts, 0);
#else
		opt = getopt(argc, argv, copyfile_opts);
#endif

		if (opt == -1)
			break;

		switch (opt)
		{
			case 'h':
				printf(copyfile_help_f, argv[0]);
				return 0;
			case 'V':
				printf("%s\n", PACKAGE_STRING);
				return 0;
			default:
				fprintf(stderr, copyfile_help_f, argv[0]);
				return 1;
		}
	}

	{
		int remain_opts = argc - optind;
		if (remain_opts != 2)
		{
			if (remain_opts < 2)
				fprintf(stderr, "%s: missing %s parameter\n", argv[0],
						remain_opts == 0 ? "SOURCE" : "DEST");
			else
				fprintf(stderr, "%s: too many parameters\n", argv[0]);

			fprintf(stderr, copyfile_help_f, argv[0]);
			return 1;
		}
	}

	{
		int ret;

		ret = copyfile_copy_file(argv[optind], argv[optind+1], 0, 0, 0);
		if (!ret)
			return 0;
		else
		{
			perror(copyfile_error_message(ret));
			return 1;
		}
	}
}
