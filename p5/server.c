#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "udp.h"

#define BUFFER_SIZE (4096)
int messagecount;

//loop waiting for data to be recieved
void receiving() {
    int sd = UDP_Open(10021);
    assert(sd > -1);
    printf("SERVER: About to enter receiver waiting loop!\r\n");
    messagecount = 0;
    while (1) {
	    struct sockaddr_in s;
	    char buffer[BUFFER_SIZE];
	    int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
	    if (rc > 0) {
	        printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
            if (buffer[0] == messagecount) {
                messagecount++;
                //idempotency -- only process messages once - always ack
                printf("SERVER processing unique message: 1-'%c' 2-'%c' with %d bytes\r\n",buffer[1],buffer[2],rc);
            }
	        char reply[BUFFER_SIZE];
            reply[0] = buffer[0]; //send ack number back with special code
            reply[1] = 'a';
            reply[2] = 'c';
            reply[3] = 'k';
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
	    }
    }
}

//local structs
typedef struct checkregion {
   void *endoflog;   
   void *imap_ptr[256];
} checkregion;

typedef struct imap {
   void *inode_ptr[16];
} imap;

typedef struct inode {
   int size; //number of the last byte in the file
   char type; //0 - file, 1 - directory
   void *data_ptr[14]; //14 direct pointers to data blocks
   //if directory data blocks contain name and inode pairs
   //name is fixed length size 60 bytes
   //inode number is just 4 bytes
   //every directory has . and .. entries
   //unused directory entries have inode number = 0
} inode;

typedef struct datablock {
   char data[64];
} datablock;

typedef struct direntry {
   char name[60];
   int inum;
} direntry;

//local variables
int portnum;
char *filesystem;
int fd;
int endoffset; //current location in file by byte offset

//Creates new fs if one does not already exist
void initializefs() {
   printf("Invalid file system specified... creating new filesystem!\r\n");
   //create new filesystem
   fd = open (filesystem, O_RDWR | O_CREAT);
   if (fd < 0) printf("Failed to create new filesystem!\r\n");
   printf("Successfully created new filesystem at: %s\r\n",filesystem);
   FILE *fp = fdopen(fd, "w");

   //create new CR region
   checkregion cr;
   cr.imap_ptr[0] = fp + sizeof(cr);
   //write(fd, &cr, sizeof(cr));
   endoffset += sizeof(cr);
   
   //initialize inode map
   imap im;
   endoffset += sizeof(im);
   //lseek(fd, endoffset, SEEK_SET);
   //write(fd, &im, sizeof(im));

   //create first inode (root directory)
   inode root;
   root.type = 1; 
   endoffset += sizeof(root);

   //create single root directory and populate (with . and ..)
   direntry dr1;
   dr1.name[0] = '.';
   dr1.inum = 0; //roots inode number
   endoffset += sizeof(dr1);

   direntry dr2;
   dr2.name[0] = '.';
   dr2.name[1] = '.';
   dr2.inum = -1;
   lseek(fd, endoffset, SEEK_SET);
   write(fd, &dr2, sizeof(dr2));
   root.data_ptr[0] = fp + endoffset; //TODO check this for sure 
   root.size = endoffset + sizeof(dr2);

   endoffset -= sizeof(dr1);
   lseek(fd, endoffset, SEEK_SET);
   write(fd, &dr1, sizeof(dr1));
   root.data_ptr[1] = fp + endoffset;
   
   endoffset -= sizeof(root); 
   lseek(fd, endoffset, SEEK_SET);
   write(fd, &root, sizeof(root));
   im.inode_ptr[0] = fp + endoffset;
   
   endoffset -= sizeof(im);
   lseek(fd, endoffset, SEEK_SET);
   write(fd, &im, sizeof(im));
   
   write(fd, &im, sizeof(im));
   printf("done writing everything out");   
}

int main(int argc, char *argv[]) {
   //check and save off input args
   receiving(); //TODO testing!!!
   /*
   if (argc != 3) {
      printf("Incorrect command line arguments: needs server [portnum] [filesystem] \r\n");
      return 1;
   } 
   portnum = atoi(argv[1]);
   filesystem = argv[2];

   //try and open filesystem -- if it doesn't exist create a new one
   fd = open(filesystem, O_RDWR);
   if (fd < 0) initializefs();
  */ 

//SUPPLIED CODE BELOW
    int sd = UDP_Open(10001);
    assert(sd > -1);

    printf("                                SERVER:: waiting in loop\n");

    while (1) {
   printf("does it actually loop");
	struct sockaddr_in s;
	char buffer[BUFFER_SIZE];
	int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
   printf("or here loop");
	if (rc > 0) {
	    printf("                                SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
	    char reply[BUFFER_SIZE];
	    sprintf(reply, "reply");
	    rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
	}
    }

    return 0;
//SUPPLIED CODE ABOVE

//close the filesystem
//fsync(fd);
//close(fd);
}


