/*
 * Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @project	mfstool
 * @program	mfstool
 * @section	1
 * @memo	Minix-Filesytem image manipulation tool.
 * @call	[global options] subcmd {img-file} [sub options]
 * @option	{img-file}
 *		Name of the Minix file system image
 * @option	-k,--check (global option)
 *		Checks filesystem state and fails if a fsck is needed.
 * @option	-q,--squash-ids (global option)
 *		Squash ids so all files are owned by root.
 * @option	--help,--usage (global option)
 *		Prints the command usage
 * @option	--version (global option)
 *		Print version information
 * @option	--copyright (global option)
 *		Print copyright information
 * @option	mkfs (sub command)
 * 		Create an empty filesystem
 * @option	-n namelen (mkfs, genfs option)
 *		specify the maximum length of filenames.  Currently,
 *		only allowable values are 14 and 30.
 * @option	-s size (mkfs, genfs option)
 *		specify the size of filesystem.
 * @option	-i inodecount (mkfs, genfs option)
 *		specify the number of inodes for the filesystem.
 * @option	-1 (mkfs, genfs option)
 *		Make a Minix version 1 filesystem.
 * @option	-2|-v (mkfs, genfs option)
 *		Make a Minix version 2 filesystem.  This is the default.
 * @option	dir (sub command)
 *		Display the contents of a directory path
 * @option	stat (sub command)
 *		Display the inode details for a file
 * @option	mkdir (sub command)
 *		Create a sub-directory
 * @option	rmdir (sub command)
 *		Remove directories
 * @option	unlink (sub command)
 *		Remove files
 * @option	cat (sub command)
 *		Equivalent of the UNIX cat command.
 * @option	extract (sub command)
 *		Extracts a file to the specified file name
 * @option	readlink (sub command)
 *		Display a symbolic link
 * @option	symlink (sub command)
 *		Create a symbolic link
 * @option	hardlink (sub command)
 *		Create a hard link
 * @option	file|dir|socket|pipe|char|block (sub command)
 *		Create a node or set file attributes
 * @option	add (sub command)
 *		Copy a file from file-system to image.
 *
 * @doc
 *		<B>mfstool</B> is a tool to manipulate Minix filesystem
 *		images.  Allows you to create arbitrary filesystem images
 *		from user space.
 *		<br>
 *		Normally it would be used for generating RAMDISK images
 *		for initrd.
 */
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#define EXTERN(a,b) a = b
#include "minix_fs.h"
#include "protos.h"
#include "config.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "minixfs"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.5.1"
#endif

#ifdef WINDOWS
int getuid()
{
  return 0;
}
int getgid()
{
  return 0;
}
int chown(const char *pathname, int owner, int group)
{
  return 0;
}
#endif

void pver() {
  printf("%s %s\n",PACKAGE_NAME,PACKAGE_VERSION);
}

void usage(const char *name) {
  pver();
  printf("Usage: %s [global options] {subcmd} {imgfile} [options]\n",name);
  exit(0);
}

void parse_opts(int *argc_p,char ***argv_p) {
  int c;
  int argc = *argc_p;
  char **argv = *argv_p;
  static struct option main_options[] = {
    { "check", no_argument, 0, 'f' },
    { "squash-ids",no_argument, 0, 'q'},
    { "help", no_argument, 0, 1000 },
    { "usage", no_argument, 0, 1000 },
    { "version", no_argument, 0, 2000},
    { "copyright", no_argument,0, 'C'},
    { 0, 0, 0, 0}
  };

  while (1) {
    int option_index = 0;
    c = getopt_long(argc,argv,"+kCq",main_options,&option_index);
    if (c == -1) break;
    switch (c) {
    case 'q':
      opt_squash = 1;
      break;
    case 'f':
      opt_fsbad_fatal = 1;
      break;
    case 1000:
      usage(argv[0]);
    case 2000:
      pver();
      exit(0);
    case 'C':
      pver();
      printf("\n\
Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>\n\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\
\n");
      exit(0);
    }
  }
  *argc_p = argc-optind;
  *argv_p = argv+optind;
}

int ftype(const char *strtype) {
  switch(strtype[0]) {
  case 's': return S_IFSOCK;
  case 'f': return S_IFREG;
  case 'b': return S_IFBLK;
  case 'd': return S_IFDIR;
  case 'c': return S_IFCHR;
  case 'p': return S_IFIFO;
  }
  return -1;
}

void do_cmd(struct minix_fs_dat *fs,int argc,char **argv) {
  if (!strcmp(argv[0],"dir")) {
    cmd_dir(fs,argc,argv);
  } else if (!strcmp(argv[0],"mkdir")) {
    cmd_mkdir(fs,argc,argv);
  } else if (!strcmp(argv[0],"rmdir")) {
    cmd_rmdir(fs,argc,argv);
  } else if (!strcmp(argv[0],"unlink")) {
    cmd_unlink(fs,argc,argv);
  } else if (!strcmp(argv[0],"cat")) {
    cmd_cat(fs,argc,argv);
  } else if (!strcmp(argv[0],"extract")) {
    cmd_extract(fs,argc,argv);
  } else if (!strcmp(argv[0],"readlink")) {
    cmd_readlink(fs,argc,argv);
  } else if (!strcmp(argv[0],"symlink")) {
    cmd_mklnk(fs,argc,argv);
  } else if (!strcmp(argv[0],"hardlink")) {
    cmd_hardlnk(fs,argc,argv);
  } else if (!strcmp(argv[0],"stat")) {
    cmd_stat(fs,argc,argv);
  } else if (!strcmp(argv[0],"add")) {
    cmd_add(fs,argc,argv);
  } else {
    int type = ftype(argv[0]);
    if (type == -1) {
      fprintf(stderr,"Invalid command: %s\n",argv[0]);
    } else {
      cmd_mknode(fs,type,argc,argv);
    }
  }
}

int main(int argc,char **argv) {
  char *cmdname = argv[0];

  parse_opts(&argc,&argv);
  if (argc < 2) usage(cmdname);

  if (!strcmp(argv[0],"mkfs")) {
    argv++,argc--;
    cmd_mkfs(argc,argv);
  } else if (!strcmp(argv[0],"genfs")) {
    argv++,argc--;
    cmd_genfs(argc,argv);
  } else {
    struct minix_fs_dat *fs = open_fs(argv[1],opt_fsbad_fatal);
    argv[1] = argv[0];
    argv++,argc--;
    do_cmd(fs,argc,argv);
    close_fs(fs);
  }
  return 0;
}
