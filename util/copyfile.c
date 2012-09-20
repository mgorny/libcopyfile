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

static const char* const copyfile_opts = "alhV";

#ifdef HAVE_GETOPT_LONG

static const struct option copyfile_long_opts[] =
{
	{ "archive", no_argument, 0, 'a' },
	{ "link", no_argument, 0, 'l' },

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
"  -a, --archive         copy file metadata as well\n"
"  -l, --link            try to create a hard link first, fall back to copy\n"
"                        (implies --archive)\n"
"\n"
"  -h, --help            print help message\n"
"  -V, --version         print program version\n";

int main(int argc, char* argv[])
{
	int opt_archive = 0;
	int opt_link = 0;

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
			case 'a':
				opt_archive = 1;
				break;
			case 'l':
				opt_link = 1;
				break;

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
		const char* source = argv[optind];
		const char* dest = argv[optind+1];

		int ret;

		if (opt_link)
			ret = copyfile_link_file(source, dest, 0, 0, 0);
		else if (opt_archive)
			ret = copyfile_archive_file(source, dest, 0, 0, 0, 0, 0);
		else
			ret = copyfile_copy_file(source, dest, 0, 0, 0);

		if (!ret)
			return 0;
		else
		{
			perror(copyfile_error_message(ret));
			return 1;
		}
	}
}
