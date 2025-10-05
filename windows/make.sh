#!/bin/sh
cd ..

x86_64-w64-mingw32-gcc -DHAVE_CONFIG_H -DWINDOWS -o ../minixfs64.exe minixfs inode.c genfs.c iname.c  reader.c super.c utils.c writer.c dir.c
i686-w64-mingw32-gcc -DHAVE_CONFIG_H -DWINDOWS -o ../minixfs32.exe minixfs.c inode.c genfs.c iname.c  reader.c super.c utils.c writer.c dir.c
zip ../minixfswin.zip ../minixfs32.exe ../minixfs64.exe
