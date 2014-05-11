#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "udp.h"
#include "mfs.h"

int fd;
int eol;
static inode *iptr;
inode inode_t;
direntry direntry_t;
char rbuf[4096];

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
   printf("FILESYSTEM MAPPING: \r\nimap_ptrs: 4\r\n");

   //write imap
   int inode_ptrs[16];
   inode_ptrs[0] = 1028 + 64;
   for (i = 1;i < 16;i++) inode_ptrs[i] = 0; 
   lseek(fd, 1028, SEEK_SET);
   write(fd, inode_ptrs, 64);
   printf("inode_ptrs: 1028\r\n"); 

   //write inode
   printf("inode: size %i - type %i - ptrs %i\r\n",1028+64,1028+64+4,1028+64+8);
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
   printf("datablock: \r\nEntry 0: %i\r\n",dptrs[0]);
   printf("Entry 1: %i\r\n",dptrs[0] + 64);

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
   eol = pt;
   lseek(fd, 0, SEEK_SET);
   write(fd, &endoflog, 4);
   fsync(fd);
   printf("eol: %i\r\n",eol);
   printf(" done!\r\n");
}

//finds next unallocated ptr
//ptr - start address of array
//returns free array locations address
int freeptr(int ptr) {
    int c = ptr;
    int temp = 1;
    while (temp != 0) {
       lseek(fd, c, SEEK_SET);  
       read(fd, &temp, 4); 
       c += 4;
    }
    return c;
}

//finds entry at imap, inode, ptr address
//returns address of location
//returns -1 if invalid
int findentry(int imap, int inode) {
    int temp; 
    int imap_a = 4 + (imap * 4); //get inode_ptrs location
    lseek(fd, imap_a, SEEK_SET); 
    read(fd, &temp, 4);   
    if (temp == 0) return -1; //temp is now ptr to inode_ptr chunck
    temp = temp + (inode * 4);
    lseek(fd, temp, SEEK_SET); 
    read(fd, &temp, 4);  //temp is now pointer to inode
    if (temp == 0) return -1;
    return temp;
}

//returns inode struct of inode with inum
//saved in global var inode_t
int getinode(int inum) {
    printf("Called getinode!\r\n");
    iptr = NULL;
    int imap = inum / 16;
    int inode = inum % 16;
    int temp;
    int sz = -1;
    int type = -1;
    printf("Grabbing imap %i and inode %i\r\n",imap,inode);
    lseek(fd, imap + 4, SEEK_SET);
    read(fd, &temp, 4);
    printf("Read pointer: %i\r\n",temp);
    if (temp == 0) return -1;
    lseek(fd, temp + (inode*4), SEEK_SET);
    read(fd, &temp, 4);
    printf("Read pointer: %i\r\n",temp);
    if (temp == 0) return -1;
    lseek(fd, temp, SEEK_SET);
    read(fd, &sz, 4);
    printf("Read inode size: %i\r\n ",sz);
    lseek(fd, temp + 4, SEEK_SET);
    read(fd, &type, 4);
    printf("Read inode type: %i\r\n ",type);
    lseek(fd, temp + 8, SEEK_SET);
    int td[14];
    read(fd, td, 56);
    inode_t.size = sz;
    inode_t.type = type;
    int i;
    for (i = 0;i < 14;i++) inode_t.data_ptrs[i] = td[i];
    return 0;
}

//loads a directory entry from ptr address
//saved in global var direntry_t
int getentry(int ptr) {
    printf("Called getentry!\r\n");
    lseek(fd, ptr, SEEK_SET);
    read(fd, &direntry_t.name, 60);
    lseek(fd, ptr+60, SEEK_SET);
    read(fd, &direntry_t.inum, 4);
    return 0;
}

//reads end of log from file and returns it
int geteol() {
    int eol;
    lseek(fd, 0, SEEK_SET);
    read(fd, &eol, 4);
    return eol;
}

//write new eol to start of file
void seteol(int eol) {
    lseek(fd, 0, SEEK_SET);
    write(fd, &eol, 4);
}


//writes data block of size bytes at location
//returns ptr to end of new location
int writeblock(int loc, char *buf, int size) {
    lseek(fd, loc, SEEK_SET);
    write(fd, buf, size);
    return loc + size;
}

//reads size bytes starting at location
//returns pointer to data
char* readblock(int loc, int size) {
    lseek(fd, loc, SEEK_SET);
    read(fd, rbuf, size);
    return rbuf;
}





