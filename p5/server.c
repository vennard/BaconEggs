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
   int size; //number of the last byte in the file?
   int type; //1 - file, 0 - directory
   int data_ptr[14]; //14 direct pointers to data blocks
   //if directory data blocks contain name and inode pairs
   //name is fixed length size 60 bytes
   //inode number is just 4 bytes
   //every directory has . and .. entries
   //unused directory entries have inode number = 0
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
   checkregion cr;
   cr.imap_ptr[0] = sizeof(cr);
   cr.eol = sizeof(cr);
   write(fd, &cr, sizeof(cr));
   printf("Eol for CR: %i!\r\n",eol);

   //initialize inode map
   eol = sizeof(cr);
   imap im;
   for(i = 0;i < 256;i++) im.inode_ptr[i] = 0;
   im.inode_ptr[0] = eol + sizeof(im);
   numinodes = 1;
   lseek(fd, eol, SEEK_SET);
   write(fd, &im, sizeof(im));
   printf("Eol for imap: %i!\r\n",eol);

   //create first inode (root directory)
   eol += sizeof(im);
   inode root;
   for(i = 0;i < 14;i++) root.data_ptr[i] = 0;
   root.size = 4096;
   //root.size = eol + sizeof(root);
   root.type = 0; 
   root.data_ptr[0] = eol + sizeof(root);
   root.data_ptr[1] = eol + sizeof(root) + sizeof(MFS_DirEnt_t);
printf("inode pointers 0: %i and 1: %i!\r\n",root.data_ptr[0],root.data_ptr[1]);
   
   lseek(fd, eol, SEEK_SET);
   write(fd, &root, sizeof(root));
   printf("Eol for inode: %i!\r\n",eol);

   //create single root directory and populate (with . and ..)
   MFS_DirEnt_t dr1;
   sprintf(dr1.name,".");
   dr1.inum = 0; //roots inode number
   eol += sizeof(root);
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr1, sizeof(dr1));
   printf("Eol for dr1: %i (Entry %s with num %i)!\r\n",eol,dr1.name,dr1.inum);

   MFS_DirEnt_t dr2;
   eol += sizeof(dr1);
   sprintf(dr2.name,"..");
   dr2.inum = dr1.inum; //can't go past this
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr2, sizeof(dr2));
   printf("Eol for dr2: %i (Entry %s with num %i)!\r\n",eol,dr2.name,dr2.inum);

   eol += sizeof(dr2);
   cr.eol = eol;
   lseek(fd, 0, SEEK_SET); 
   write(fd, &cr, sizeof(cr)); 
   printf("done writing everything out");   
   printf("final eol: %i!\r\n",eol);

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


