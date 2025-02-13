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
 * @module	genfs
 * @section	3
 * @doc	routines for creating new file systems
 */
#include "minix_fs.h"
#include "protos.h"
#include <getopt.h>
#include <stdlib.h>

/**
 * Parse mkfs/genfs command line arguments
 * @param argc - argc from command line
 * @param argv - argv from command line
 * @param magic_p - returns filesystem magic number
 * @param nblks_p - returns size of file system
 * @param inodes_p - return number of requested inodes
 */
/* 
 * -n namelen
 * -1 -> version1
 * -2|v -> version2
 * -i nodecount
 * -s nblocks
 * -k
 */
void parse_mkfs(int argc,char **argv,int *magic_p,int *nblks_p,int *inodes_p,int *keepdata_p) {
  int c;
  int namelen = 30;
  int version = 2;
  *nblks_p = -1;
  *inodes_p = 0;
  *keepdata_p = 0;
  while (1) {
    c = getopt(argc,argv,"12vki:n:s:");
    if (c == -1) break;
    switch (c) {
    case '1':
      version = 1;
      break;
    case '2':
    case 'v':
      version = 2;
      break;
    case 'n':
      namelen = atoi(optarg);
      if (namelen != 30 && namelen != 14)
        fatalmsg("invalid name len (%d) must be 30 or 14",namelen);
    case 'i':
      *inodes_p = atoi(optarg);
      break;
    case 's':
      *nblks_p = atoi(optarg);
      break;
    case 'k':
      *keepdata_p = 1;
      break;
    default:
      usage(argv[0]);
    }
  }
  if (*nblks_p == -1)
    fatalmsg("no filesystem size specified");

  if (version == 2)
    *magic_p = namelen == 30 ? MINIX2_SUPER_MAGIC2 : MINIX2_SUPER_MAGIC;
  else
    *magic_p = namelen == 30 ? MINIX_SUPER_MAGIC2 : MINIX_SUPER_MAGIC;
}

/**
 * Create an empty filesystem
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mkfs(int argc,char **argv) {
  int req_inos;
  int req_blks;
  int magic;
  int keepdata;
  char *filename = argv[0];
  struct minix_fs_dat *fs;

  parse_mkfs(argc,argv,&magic,&req_blks,&req_inos,&keepdata);
  fs = new_fs(filename,magic,req_blks,req_inos,keepdata);
  close_fs(fs);
}

/**
 * Generate a new filesystem
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_genfs(int argc,char **argv) {
  int req_inos;
  int req_blks;
  int magic;
  int keepdata;
  char *filename = argv[0];
  struct minix_fs_dat *fs;

  parse_mkfs(argc,argv,&magic,&req_blks,&req_inos,&keepdata);
  fs = new_fs(filename,magic,req_blks,req_inos,keepdata);
  {
    int i;
    for (i=optind;i<argc;i++) {
      printf("%3d) %s\n",i,argv[i]);
    }
  }
  close_fs(fs);
}
