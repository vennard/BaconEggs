#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)
int messagecount;
char buffer[BUFFER_SIZE];

//loop waiting for data to be recieved
void receiving() {
    int sd = UDP_Open(10021);
    assert(sd > -1);
    printf("SERVER: About to enter receiver waiting loop!\r\n");
    messagecount = 0;
    while (1) {
	    struct sockaddr_in s;
	    int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
	    if (rc > 0) {
	        printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
            if ((buffer[BUFFER_SIZE-3] == messagecount)&&(buffer[BUFFER_SIZE-2] == 'k')&&(buffer[BUFFER_SIZE-1] == 'z'))  {
                messagecount++;
                //idempotency -- only process messages once - always ack
                printf("SERVER processing unique message (%d bytes)!\r\n",rc);
            }
	         char reply[BUFFER_SIZE];
            reply[0] = buffer[BUFFER_SIZE-3]; //send ack number back with special code
            reply[1] = 'a';
            reply[2] = 'c';
            reply[3] = 'k';
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
	    }
    }
}

//local variables
int portnum;
char *filesystem;
int fd;
int eol; //current location in file by byte offset
int numinodes;

void startfs() {
   printf("Creating new filesystem... ");
   int i;

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
   //dptrs[1] = 1028 + 64 + 64 + 64;
   for (i = 2;i < 14;i++) dptrs[i] = 0;
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

   //TODO Assuming 4KB directory block -- filling in -1 inums
   int pt = 1028 + (64 * 4);
   inum = -1;
   for (i = 2;i < 64;i++) {
      lseek(fd, pt + 60, SEEK_SET); 
      write(fd, &inum, 4);
      pt += 64;
   }
/*
   //TODO attempting to stop invalidate directory entries
   int marker = 1028 + (64 * 4);
   inum = -1;
   for (i = 2;i < 14;i++) {
      lseek(fd, marker+60, SEEK_SET);
      write(fd, &inum, 4);
      marker += 64;
   }
*/
   //write end of log
   int endoflog = 1028 + 64 + 64 + 64 + 64;
   lseek(fd, 0, SEEK_SET);
   write(fd, &endoflog, 4);
   fsync(fd);
   printf(" done!\r\n");
}

int main(int argc, char *argv[]) {
   //receiving();

   //check and save off input args
   if (argc != 3) {
      printf("Incorrect command line arguments: needs server [portnum] [filesystem] \r\n");
      return 1;
   } 
   portnum = atoi(argv[1]);
   filesystem = argv[2];

   //try and open filesystem -- if it doesn't exist create a new one
   fd = open(filesystem, O_RDWR);
   if (fd < 0) startfs();

   close(fd);
   return 0;
}


