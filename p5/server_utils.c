#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "udp.h"
#include "mfs.h"

int fd;

void startfs(char* filesystem) {
   int i;
   printf("Creating new filesystem... ");
   fd = open(filesystem, O_RDWR | O_CREAT);
   if (fd < 0) printf("Failed to create new filesystem!\r\n");

   //write check region imap ptrs 
   int imap_ptrs[256];
   imap_ptrs[0] = 1028;
   for (i = 1;i < 256;i++) imap_ptrs[i] = 0; 
   lseek(fd, 4, SEEK_SET);
   write(fd, imap_ptrs, 1024);

   //write imap
   int inode_ptrs[16];
   inode_ptrs[0] = 1028 + 64;
   for (i = 1;i < 16;i++) inode_ptrs[i] = 0; 
   lseek(fd, 1028, SEEK_SET);
   write(fd, inode_ptrs, 64);

   //write inode
   int size = 4096;
   lseek(fd, 1028 + 64, SEEK_SET);
   write(fd, &size, 4);
   int type = 0;
   lseek(fd, 1028 + 64 + 4, SEEK_SET);
   write(fd, &type, 4);
   int dptrs[14];
   dptrs[0] = 1028 + 64 + 64;
   for (i = 1;i < 14;i++) dptrs[i] = 0;
   lseek(fd, 1028 + 64 + 8, SEEK_SET);
   write(fd, dptrs, 56);

   //directory entry 1
   char name[60];
   sprintf(name, ".");
   int inum = 0;
   lseek(fd, 1028 + 64 + 64, SEEK_SET);
   write(fd, name, 60);
   lseek(fd, 1028 + 64 + 64 + 60, SEEK_SET);
   write(fd, &inum, 4);
   //directory entry 2
   sprintf(name, "..");
   lseek(fd, 1028 + 64 + 64 + 64, SEEK_SET);
   write(fd, name, 60);
   lseek(fd, 1028 + 64 + 64 + 64 + 60, SEEK_SET);
   write(fd, &inum, 4);

   //4KB directory block -- filling in -1 to invalid inums
   int pt = 1028 + (64 * 4);
   inum = -1;
   for (i = 2;i < 64;i++) {
      lseek(fd, pt + 60, SEEK_SET); 
      write(fd, &inum, 4);
      pt += 64;
   }

   //write end of log
   int endoflog = pt;
   lseek(fd, 0, SEEK_SET);
   write(fd, &endoflog, 4);
   fsync(fd);
   printf(" done!\r\n");
}


