AUTOMAKE_OPTIONS = gnu

MAINTAINERCLEANFILES = INSTALL Makefile.in aclocal.m4 autoscan.log \
			configure depcomp install-sh missing protos.h \
			config.h.in ChangeLog minixfs.1
man1_MANS = minixfs.1
EXTRA_DIST = $(man1_MANS)

bin_PROGRAMS = minixfs
minixfs_SOURCES = minixfs.c super.c utils.c inode.c genfs.c dir.c reader.c \
		writer.c iname.c
noinst_HEADERS = minix_fs.h bitops.h protos.h

protos:
	cheaders $(minixfs_SOURCES) | sed 's/int \.\.\./.../' > protos.h

clog:
	echo "Empty change log"  > ChangeLog

mans:
	cdoc minixfs.c

