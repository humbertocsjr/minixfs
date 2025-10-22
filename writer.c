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
 * @module	writer
 * @section	3
 * @doc	routines for writing files
 */
#include "minix_fs.h"
#include "protos.h"
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Write to a file/inode.  It makes holes along the way...
 * @param fs - File system structure
 * @param fp - Input file
 * @param inode - Inode to write to
 */
void writefile(struct minix_fs_dat *fs,FILE *fp,int inode) {
  int j,bsz,blkcnt = 0;
  u8 blk[BLOCK_SIZE];
  u32 count = 0;
  do {
    bsz = fread(blk,1,BLOCK_SIZE,fp);
    for (j=0;j<bsz;j++) if (blk[j]) break;
    //if (j != bsz) { /* This is not a hole, so better write it */
      if (bsz < BLOCK_SIZE) memset(blk+bsz,0,BLOCK_SIZE-bsz);
      write_inoblk(fs,inode,blkcnt++,blk);
    //} else {
    //    printf("???");
    //  free_inoblk(fs,inode,blkcnt++);
    //}
    count += bsz;
  } while (bsz == BLOCK_SIZE);
  trunc_inode(fs,inode,count);
}

/**
 * Write data to file/inode.
 * @param fs - File system structure
 * @param blk - Data to write
 * @param cnt - bytes to write
 * @param inode - Inode to write to
 */
void writedata(struct minix_fs_dat *fs,u8 *blk,u32 cnt,int inode) {
  int i,blkcnt;
  
  for (blkcnt=i=0; i < cnt; i+= BLOCK_SIZE) {
    if (i+BLOCK_SIZE < cnt) {
      write_inoblk(fs,inode,blkcnt++,blk+i);
    } else {
      u8 blk2[BLOCK_SIZE];
      memcpy(blk2,blk+i,cnt-i);
      memset(blk2+cnt-i,0,BLOCK_SIZE-cnt+i);
      write_inoblk(fs,inode,blkcnt,blk2);
    }
  }
  trunc_inode(fs,inode,cnt);
}



/**
 * Create a symlink
 * @param fs - file system structure
 * @param target - target link
 * @param lnknam - link name
 */
void domklnk(struct minix_fs_dat *fs,char *target,char *lnknam) {
  int len = strlen(target);
  int inode = make_node(fs,lnknam,0777|S_IFLNK,0,0,len,NOW,NOW,NOW,NULL);
  writedata(fs,target,len,inode);
}

/**
 * Create a symlink command
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mklnk(struct minix_fs_dat *fs,int argc,char **argv) {
  if (argc == 3) fatalmsg("Usage: %s [link target] [link name]\n",argv[0]);
  domklnk(fs,argv[1],argv[2]);
}

/**
 * Create a hard link
 * @param fs - file system structure
 * @param target - target link
 * @param lnknam - link name
 */
void domkhlnk(struct minix_fs_dat *fs,char *target,char *lnknam) {
  char *dir = lnknam;
  char *lname = strrchr(lnknam,'/');
  int dinode;
  int tinode = find_inode(fs,target);

  if (VERSION_2(fs)) {
    struct minix2_inode *ino =INODE2(fs,tinode);
    if (!S_ISREG(ino->i_mode)) fatalmsg("%s: can only link regular files");
    ino->i_nlinks++;
  } else {
    struct minix_inode *ino =INODE(fs,tinode);
    if (!S_ISREG(ino->i_mode)) fatalmsg("%s: can only link regular files");
    ino->i_nlinks++;
  }  
  if (find_inode(fs,lnknam) != -1) fatalmsg("%s: already exists",lnknam); 
  if (lnknam) {
    *(lnknam++) = 0;
  } else {
    dir = ".";
    lname = lnknam;
  }
  dinode = find_inode(fs,dir);
  if (dinode == -1) fatalmsg("%s: not found\n",dir);
  dname_add(fs,dinode,lname,tinode);
}

/**
 * Create a symlink command
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_hardlnk(struct minix_fs_dat *fs,int argc,char **argv) {
  if (argc == 3) fatalmsg("Usage: %s [link target] [link name]\n",argv[0]);
  domklnk(fs,argv[1],argv[2]);
}



/**
 * Create a nodes
 * @param fs - file system structure
 * @param type - device type
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mknode(struct minix_fs_dat *fs,int type,int argc,char **argv) {
  int mode;
  int uid = dogetuid(),gid = dogetgid();
  int major = 0, minor = 0, count = 0,inc = 1;
  if (type == S_IFBLK || type == S_IFCHR ? 
	argc < 7 || argc > 9 : argc < 3 || argc > 5)
    fatalmsg("Usage: %s path mode [uid gid [major minor [count [inc]]]] %d\n",
		argv[0], argc);
  
  mode = (strtoul(argv[2],NULL,8) & 07777) | type;
  if (argc >= 4) uid = atoi(argv[3]);
  if (argc >= 5) gid = atoi(argv[4]);
  if (argc >= 6) major = atoi(argv[5]);
  if (argc >= 7) minor = atoi(argv[6]);
  if (argc >= 8) count = atoi(argv[7]);
  if (argc >= 9) inc = atoi(argv[8]);
  if (inc < 1) fatalmsg("Invalid increment value: %d\n",inc);

  if (count && (type == S_IFBLK || type == S_IFCHR)) {
    char b[BLOCK_SIZE];
    int i = 0;
    do {
      snprintf(b,sizeof b,"%s%d",argv[1],i++);
      make_node(fs,b,mode,uid,gid,DEVNUM(major,minor),NOW,NOW,NOW,NULL);
      minor += inc;
    } while (minor < count);
  } else {
    if (type == S_IFREG) {
      int fnode = find_inode(fs,argv[1]);
      if (fnode == -1) fatalmsg("%s: not found\n",argv[1]);
      if (VERSION_2(fs)) {
	if (!S_ISREG(INODE2(fs,fnode)->i_mode))
	  fatalmsg("%s: not a regular file\n",argv[1]);
        INODE2(fs,fnode)->i_uid = uid;
        INODE2(fs,fnode)->i_gid = gid;
	INODE2(fs,fnode)->i_mode = mode;
      } else {
	if (!S_ISREG(INODE(fs,fnode)->i_mode))
	  fatalmsg("%s: not a regular file\n",argv[1]);
        INODE(fs,fnode)->i_uid = uid;
        INODE(fs,fnode)->i_gid = gid;
	INODE(fs,fnode)->i_mode = mode;
      }
    } else if (type == S_IFDIR) {
      int dnode = domkdir(fs,argv[1]);
      if (VERSION_2(fs)) {
        INODE2(fs,dnode)->i_uid = uid;
        INODE2(fs,dnode)->i_gid = gid;
      } else {
        INODE(fs,dnode)->i_uid = uid;
        INODE(fs,dnode)->i_gid = gid;
      }
    } else {
      make_node(fs, argv[1], mode, uid, gid, DEVNUM(major,minor), 
			NOW, NOW, NOW, NULL);
    }
  }
}

/**
 * Add files to a image file
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_add(struct minix_fs_dat *fs,int argc,char **argv) {
  FILE *fp;
  struct stat sb;
  int inode;

  if (argc != 3) fatalmsg("Usage: %s [image] [input file] [image file]\n",argv[0]);
  if (stat(argv[1],&sb)) die("stat(%s)",argv[1]);
  if (!S_ISREG(sb.st_mode)) fatalmsg("%s: not a regular file\n",argv[1]);

  fp = fopen(argv[1],"rb");
  if (!fp) die(argv[1]);

  inode = make_node(fs,argv[2],sb.st_mode,
			opt_squash ? 0 : sb.st_uid,opt_squash ? 0 : sb.st_gid,
			sb.st_size,sb.st_atime,sb.st_mtime,sb.st_ctime,NULL);
  
  writefile(fs,fp,inode);
  fclose(fp);
}


