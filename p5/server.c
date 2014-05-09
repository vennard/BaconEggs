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

//local structs
typedef struct checkregion {
   int eol;
   int imap_ptr[256];
} checkregion;

typedef struct imap {
   int inode_ptr[16];
} imap;

typedef struct inode {
   MFS_Stat_t stat;
   //int size;
   //int type;
   int data_ptr[14]; //14 direct pointers to data blocks
   //if directory data blocks contain name and inode pairs
   //name is fixed length size 60 bytes
   //inode number is just 4 bytes
} inode;

typedef struct datablock {
   char data[64];
} datablock;

//local variables
int portnum;
char *filesystem;
int fd;
int eol; //current location in file by byte offset
int numinodes;

//Creates new fs if one does not already exist
void initializefs() {
   int i;
   printf("Invalid file system specified... creating new filesystem!\r\n");
   //create new filesystem
   fd = open(filesystem, O_RDWR | O_CREAT);
   if (fd < 0) printf("Failed to create new filesystem!\r\n");
   printf("Successfully created new filesystem at: %s\r\n",filesystem);

   //create new CR region
   eol = 0;
   int iptrs[256];
   iptrs[0] = 1028;
   lseek(fd, 4, SEEK_SET);
   write(fd, iptrs, 1024);
/*
   checkregion cr;
   cr.imap_ptr[0] = sizeof(cr);
   cr.eol = sizeof(cr);
   write(fd, &cr, sizeof(cr));
   printf("CR: %i (%i bytes)\r\n",eol,sizeof(cr));
   printf("     imap_ptr[0] - %i\r\n",cr.imap_ptr[0]);
*/
   //initialize inode map
   eol = 1028; 
   imap im;
   for(i = 0;i < 256;i++) im.inode_ptr[i] = 0;
   im.inode_ptr[0] = eol + sizeof(im);
   numinodes = 1;
   lseek(fd, eol, SEEK_SET);
   write(fd, &im, sizeof(im));
   printf("imap: %i (%i bytes)\r\n",eol,sizeof(im));
   printf("       inode_ptr[0] - %i\r\n",im.inode_ptr[0]);

   //create first inode (root directory)
   eol += 64;
   MFS_Stat_t st;
   st.type = 0;
   st.size = 4096;
   int size = 4096;
   int type = 0;
   int ptrs[14];
   ptrs[0] = eol + 64;
   ptrs[1] = eol + 64 + 64;
   for (i = 2; i < 14;i++) ptrs[i] = 0;
   lseek(fd, eol, SEEK_SET);
   //write(fd, &st,sizeof(st));
   write(fd, &size, 4);
   lseek(fd, eol+4, SEEK_SET);
   write(fd, &type, 4);
   lseek(fd, eol+8, SEEK_SET);
   write(fd, ptrs, 56);
   printf("inode: %i (64 bytes)\r\n",eol);
   printf("        ptr[0] - %i\r\n",ptrs[0]);
   printf("        ptr[1] - %i\r\n",ptrs[1]);

   /*
   eol += sizeof(im);
   inode root;
   for(i = 2;i < 14;i++) root.data_ptr[i] = 0;
   root.stat.size = 4096;
   //root.size = 4096;
   root.stat.type = 0; 
   //root.type = 0;
   root.data_ptr[0] = eol + sizeof(root);
   root.data_ptr[1] = eol + sizeof(root) + sizeof(MFS_DirEnt_t);
   //root.data_ptr[2] = sizeof(MFS_DirEnt_t);
   printf("inode pointers 0: %i and 1: %i!\r\n",root.data_ptr[0],root.data_ptr[1]);
   lseek(fd, eol, SEEK_SET);
   write(fd, &root, sizeof(root));
   printf("Eol for inode: %i!\r\n",eol);

*/
   //create single root directory and populate (with . and ..)
   eol += 64;
   int inu = 0;
   char bf[60];
   bf[0] = '.';
   bf[1] = '.';
   bf[2] = '\0';
   lseek(fd, eol, SEEK_SET);
   write(fd, &inu,4);
   lseek(fd, eol+4, SEEK_SET);
   write(fd, bf,60);
/*
   MFS_DirEnt_t dr1;
   dr1.inum = 0; //roots inode number
   //sprintf(dr1.name,"SOMETHINGS HAS TO HAPPEN-----------------------");
   sprintf(dr1.name,".");
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr1, sizeof(dr1));
*/
   printf("dirent 1: %i (%i bytes)\r\n",eol, 64);

   MFS_DirEnt_t dr2;
   eol += sizeof(dr2);
   //sprintf(dr2.name,".................................\0");
   sprintf(dr2.name,"..");
   dr2.inum = 0; //can't go past this
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr2, sizeof(dr2));
   printf("dirent 2: %i (%i bytes)\r\n",eol, sizeof(dr2));

   eol += sizeof(dr2);
   lseek(fd, 0, SEEK_SET); 
   write(fd, &eol, 4); 
   printf("done writing everything out");   
   printf("final eol: %i!\r\n",eol);



   printf("------------read back check----------\r\n");
   lseek(fd, 4, SEEK_SET);
   int cimap, cinode, csize, ctype, cd1, cd2;
   read(fd, &cimap, 4);
   printf("check region ptr to imap gave: %i\r\n",cimap);
   lseek(fd, cimap, SEEK_SET);
   read(fd, &cinode, 4);
   printf("imap ptr to inode gave: %i\r\n",cinode);
   lseek(fd, cinode, SEEK_SET);
   read(fd, &csize, 4);
   lseek(fd, cinode+4, SEEK_SET);
   read(fd, &ctype, 4);
   lseek(fd, cinode+8, SEEK_SET);
   read(fd, &cd1, 4);
   printf("inode - size: %i type: %i\r\n",csize,ctype);
   printf("        ptr to direntry 1: %i \r\n",cd1);
   
  

   fsync(fd);
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
   if (fd < 0) initializefs();


   close(fd);
   return 0;
}


