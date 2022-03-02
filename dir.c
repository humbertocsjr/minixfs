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
 * @module	dir
 * @section	3
 * @doc	routines for reading directories
 */
#include "minix_fs.h"
#include "protos.h"
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/**
 * Print a directory entry
 * @param fp - file to write to (typically stdio)
 * @param dp - pointer to directory entry's name
 * @param namlen - length of file name
 */
void outent(FILE *fp,const char *dp,int namlen) {
  while (*dp && namlen--) {
    putc(*dp++,fp);
  }
  putc('\n',fp);
}

/**
 * List contents of a directory
 * @param fs - filesystem structure
 * @param path - directory path to list
 */
void dodir(struct minix_fs_dat *fs,const char *path) {
  int inode = find_inode(fs,path);
  int i,bsz,j;
  u8 blk[BLOCK_SIZE];
  int fdirsize;
  int dentsz = DIRSIZE(fs);

  if (inode == -1) fatalmsg("%s: not found",path);
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",path);
    fdirsize = ino->i_size;    
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",path);
    fdirsize = ino->i_size;    
  }
  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j+= dentsz) {
      u16 fino = *((u16 *)(blk+j));
      // printf("%d ",fino);
      if (fino == 0) continue;
      outent(stdout,blk+j+2,dentsz-2);
    }
  }
}


/**
 * Commant to list contents of a directory
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_dir(struct minix_fs_dat *fs,int argc,char **argv) {
  if (argc == 1) {
    dodir(fs,".");
  } else if (argc == 2) {
    dodir(fs,argv[1]);
  } else {
    int i;
    for (i=1;i<argc;i++) {
      printf("%s:\n",argv[i]);
      dodir(fs,argv[i]);
    }
  }
}

/**
 * Create a directory
 * @param fs - filesystem structure
 * @param newdir - directory name
 */
int domkdir(struct minix_fs_dat *fs,char *newdir) {
  int dinode;
  int ninode = make_node(fs,newdir,0755|S_IFDIR,dogetuid(),dogetgid(), 0,
  			NOW,NOW,NOW,&dinode);
  dname_add(fs,ninode,".",ninode);
  dname_add(fs,ninode,"..",dinode);
  if (VERSION_2(fs)) {
    INODE2(fs,dinode)->i_nlinks++;
    INODE2(fs,ninode)->i_nlinks++;
  } else {
    INODE(fs,dinode)->i_nlinks++;
    INODE(fs,ninode)->i_nlinks++;
  }
  return ninode;
}

/**
 * Command to create directories
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mkdir(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    domkdir(fs,argv[i]);
  }
}


/**
 * Print time formatted
 * @param fp - output file pointer
 * @param str - type string
 * @param usecs - time stamp
 */
void timefmt(FILE *fp,const char *str,u32 usecs) {
  struct tm tmb;
  time_t secs = usecs;
#ifdef WINDOWS
  gmtime_s(&secs,&tmb);
#else
  gmtime_r(&secs,&tmb);
#endif
  fprintf(fp,"\t%s=%04d/%02d/%02d %02d:%02d:%02d (GMT)\n",
  		str,
		tmb.tm_year + 1900, tmb.tm_mon+1, tmb.tm_mday,
		tmb.tm_hour, tmb.tm_min, tmb.tm_sec);
}
/**
 * Print formatted mode value
 * @param fp - output file
 * @param mode - mode bits
 */
void outmode(FILE *fp,int mode) {
  switch (mode & S_IFMT) {
    case S_IFSOCK: fprintf(fp,"\ttype=socket\n"); break;
    case S_IFLNK: fprintf(fp,"\ttype=symbolic link\n"); break;
    case S_IFREG: fprintf(fp,"\ttype=regular file\n"); break;
    case S_IFBLK: fprintf(fp,"\ttype=block device\n"); break;
    case S_IFDIR: fprintf(fp,"\ttype=directory\n"); break;
    case S_IFCHR: fprintf(fp,"\ttype=character device\n"); break;
    case S_IFIFO: fprintf(fp,"\ttype=fifo\n"); break;
    default: fprintf(fp,"\ttype=unknown (%o)\n",mode & S_IFMT);
  }
  fprintf(fp,"\tmode=%04o\n",mode & 07777);
}

/**
 * Print stat entries for a inode
 * @param fs - filesystem structure
 * @param path - directory path to list
 */
void dostat(struct minix_fs_dat *fs,const char *path) {
  int inode = find_inode(fs,path);
  if (inode == -1) {
    fprintf(stderr,"%s: not found\n",path);
    return;
  }

  printf("\tinode=%d\n",inode);
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    outmode(stdout,ino->i_mode);
    printf("\tnlinks=%d\n",ino->i_nlinks);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode))
      printf("\tmajor=%d,minor=%d\n",
		(ino->i_zone[0]>>8) & 0xff,
		ino->i_zone[0] & 0xff);
    else
      printf("\tsize=%d\n",ino->i_size);
    printf("\tuid=%d\n\tgid=%d\n",ino->i_uid,ino->i_gid);
    timefmt(stdout,"accessed",ino->i_atime);
    timefmt(stdout,"modified",ino->i_mtime);
    timefmt(stdout,"changed",ino->i_ctime);
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    outmode(stdout,ino->i_mode);
    printf("\tnlinks=%d\n",ino->i_nlinks);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode))
      printf("\tmajor=%d\nminor=%d\n",
		(ino->i_zone[0]>>8) & 0xff,
		ino->i_zone[0] & 0xff);
    else
      printf("\tsize=%d\n",ino->i_size);
    printf("\tuid=%d\n\tgid=%d\n",ino->i_uid,ino->i_gid);
    timefmt(stdout,"accessed",ino->i_time);
  }
}

/**
 * Command to stat entries
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_stat(struct minix_fs_dat *fs,int argc,char **argv) {
  if (argc == 1) {
    dostat(fs,".");
  } else if (argc == 2) {
    dostat(fs,argv[1]);
  } else {
    int i;
    for (i=1;i<argc;i++) {
      printf("%s:\n",argv[i]);
      dostat(fs,argv[i]);
    }
  }
}

/**
 * Remove an <b>empty</b> directory
 * @param fs - filesystem structure
 * @param dir - directory to remove
 */
void dormdir(struct minix_fs_dat *fs,const char *dir) {
  int inode = find_inode(fs,dir);
  int i,bsz,j;
  u8 blk[BLOCK_SIZE];
  int fdirsize;
  int dentsz = DIRSIZE(fs);
  int pinode = -1;
  const char *p;

  if (inode == -1) fatalmsg("%s: not found",dir);
  if (inode == MINIX_ROOT_INO) fatalmsg("Can not remove root inode");

  /* Make sure directory is a directory */
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",dir);
    fdirsize = ino->i_size;    
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",dir);
    fdirsize = ino->i_size;    
  }

  /* Do a directory scan... */
  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j+= dentsz) {
      u16 fino = *((u16 *)(blk+j));
      if (blk[j+2] == '.' && blk[j+3] == 0) continue;
      if (blk[j+2] == '.' && blk[j+3] == '.'&& blk[j+4] == 0) {
        pinode = fino;
        continue;
      }
      if (fino != 0) fatalmsg("%s: not empty",dir);
    }
  }

  /* Free stuff */
  trunc_inode(fs,inode,0);
  clr_inode(fs, inode);
  if (VERSION_2(fs)) {
    INODE(fs,pinode)->i_nlinks--;
  } else {
    INODE(fs,pinode)->i_nlinks--;
  }
  p = strrchr(dir,'/');
  if (p)
    p++;
  else
    p = dir;
  dname_rem(fs,pinode,p);
}

/**
 * Remove directories
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_rmdir(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    dormdir(fs,argv[i]);
  }
}

/**
 * Remove an file (not directory)
 * @param fs - filesystem structure
 * @param fpath - file path
 */
void dounlink(struct minix_fs_dat *fs,char *fpath) {
  char *dir = fpath;
  char *fname = strrchr(fpath,'/');
  int dinode,inode;

  if (fname) {
    *(fname++) = 0;
  } else {
    dir = ".";
    fname = fpath;
  }
  dinode = find_inode(fs,dir);
  if (dinode == -1) fatalmsg("%s: not found\n",dir);
  inode = ilookup_name(fs,dinode,fname,NULL,NULL);
  if (inode == -1) fatalmsg("%s: not found\n",fname);

  /* Make sure file is not a directory */
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (S_ISDIR(ino->i_mode)) fatalmsg("%s: is a directory",dir);
    dname_rem(fs,dinode,fname);
    if (--(ino->i_nlinks)) return;
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is a directory",dir);
    dname_rem(fs,dinode,fname);
    if (--(ino->i_nlinks)) return;
  }
  /* Remove stuff... */
  trunc_inode(fs,inode,0);
  clr_inode(fs,inode);
}

/**
 * Remove files (but not directories)
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_unlink(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    dounlink(fs,argv[i]);
  }
}
