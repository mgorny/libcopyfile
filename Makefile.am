lib_LTLIBRARIES = src/libcopyfile.la

src_libcopyfile_la_SOURCES = src/copyfile.c src/copyfile.h
src_libcopyfile_la_LDFLAGS = -version-info 0:0:0

EXTRA_DIST = NEWS
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
