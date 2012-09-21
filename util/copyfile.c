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

static const char* const copyfile_opts = "almPhV";

#ifdef HAVE_GETOPT_LONG

static const struct option copyfile_long_opts[] =
{
	{ "archive", no_argument, 0, 'a' },
	{ "link", no_argument, 0, 'l' },
	{ "progress", no_argument, 0, 'P' },

	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },

	{ 0, 0, 0, 0 }
};

#endif /*HAVE_GETOPT_LONG*/

static const char* const copyfile_help_usage =
"Usage: %s [OPTIONS] SOURCE DEST\n"
"\n"
"Copy a single file SOURCE to a new full path DEST. DEST must not\n"
"be just a directory, it has to contain the filename as well.\n"
"\n";

static const char* const copyfile_help_options =
"  -a, --archive         copy file metadata as well\n"
"  -l, --link            try to create a hard link first, fall back to copy\n"
"                        (implies --archive)\n"
"  -m, --move            move (rename) instead of copying, fall back to copy\n"
"                        and remove (implies --archive)\n"
"  -P, --progress        enable verbose progress reporting\n"
"\n"
"  -h, --help            print help message\n"
"  -V, --version         print program version\n";

static const char* const ecma_prev_line = "\033[A";
static const char* const progress_bar =
		"=================================";
static const char* const progress_spaces =
		"                                 ";

static int progress_callback(copyfile_error_t state,
		copyfile_filetype_t ftype, copyfile_progress_t prog,
		void* data, int default_return)
{
	if (ftype == COPYFILE_REGULAR)
	{
		unsigned int perc, bar_blocks;

		prog.data.offset >>= 10;
		prog.data.size >>= 10;

		perc = (prog.data.offset * 100) / prog.data.size;
		bar_blocks = perc / 3;

		if (state == COPYFILE_EOF || prog.data.offset != 0)
			fputs(ecma_prev_line, stderr);

		fprintf(stderr, "%7lu / %7lu KiB (%3u%%) [%s%c%s]\n",
				prog.data.offset, prog.data.size, perc,
				progress_bar + 33 - bar_blocks,
				state != COPYFILE_EOF ? '>' : '=',
				progress_spaces + bar_blocks);

	}

	return default_return;
}

int main(int argc, char* argv[])
{
	int opt_archive = 0;
	int opt_link = 0;
	int opt_move = 0;

	copyfile_callback_t opt_progress = 0;

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
			case 'm':
				opt_move = 1;
				break;
			case 'P':
				opt_progress = progress_callback;
				break;

			case 'h':
				printf(copyfile_help_usage, argv[0]);
				fputs(copyfile_help_options, stdout);
				return 0;
			case 'V':
				printf("%s\n", PACKAGE_STRING);
				return 0;
			default:
				fprintf(stderr, copyfile_help_usage, argv[0]);
				fputs(copyfile_help_options, stderr);
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

			fprintf(stderr, copyfile_help_usage, argv[0]);
			fputs(copyfile_help_options, stderr);
			return 1;
		}

		if (opt_move && opt_link)
		{
			fprintf(stderr, "%s: --link and --move must not be used together\n",
					argv[0]);
			return 1;
		}
	}

	{
		const char* source = argv[optind];
		const char* dest = argv[optind+1];

		int ret;

		if (opt_move)
			ret = copyfile_move_file(source, dest, 0, opt_progress, 0);
		else if (opt_link)
			ret = copyfile_link_file(source, dest, 0, opt_progress, 0);
		else if (opt_archive)
			ret = copyfile_archive_file(source, dest, 0, 0, 0,
					opt_progress, 0);
		else
			ret = copyfile_copy_file(source, dest, 0, opt_progress, 0);

		if (!ret)
			return 0;
		else
		{
			perror(copyfile_error_message(ret));
			return 1;
		}
	}
}
