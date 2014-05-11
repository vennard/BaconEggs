#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "udp.h"
#include "mfs.h"

#define debug (1)

#define BUFFER_SIZE (4107)
#define COMMAND_BYTE (BUFFER_SIZE-9)
#define DATA_BLOCK (0)
#define KEY_BYTE (BUFFER_SIZE-11)
#define MESSAGE_ID (BUFFER_SIZE-10)
#define CMD_INT1 (BUFFER_SIZE-8)
#define CMD_INT2 (BUFFER_SIZE-4)
#define TIMEOUT (3)

int messagecount;
char buffer[BUFFER_SIZE];
char reply[BUFFER_SIZE];
char data[4097];

//loop waiting for data to be recieved
void receiving(int port) {
    char ackd[] = {'a','c','k','d'};

    //OPEN UDP PORT
    int sd = UDP_Open(port);
    if (sd < 0){
        printf("SERVER : There was an error opening port %d. Exiting.\r\n", port);
	exit(-1);
    }

    if (debug) printf("SERVER : About to enter receiver waiting loop!\r\n");
    
    //MAIN RECEIVING LOOP
    while (1) {
	    struct sockaddr_in s;

	    //READ A PACKET FROM THE UDP PORT
	    int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
	    if (rc > 0) {
	        printf("SERVER : read %d bytes.\n", rc);
	    	printf("Recieved command %d, messageid %d, key value %c, command int one %d, command int two %d.\n", buffer[COMMAND_BYTE], buffer[MESSAGE_ID], buffer[KEY_BYTE], buffer[CMD_INT1], buffer[CMD_INT2]);
		memcpy(data, &buffer[0], 4096);

		//FORMAT DATA AS STRING TO PRINT, DON'T INCLUDE NORMALLY
		data[4096] = '\0';

		printf("Data: %s\n", data);
            	reply[COMMAND_BYTE] = buffer[COMMAND_BYTE];
           	reply[MESSAGE_ID] = buffer[MESSAGE_ID];
            	reply[KEY_BYTE] = 'k'; //reset key value
	    	memcpy(&reply[CMD_INT1], ackd, 4); //"ackd"
	    	reply[CMD_INT2] = 0; //return value
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
   
   //initialize inode map
   eol = sizeof(cr);
   imap im;
   im.inode_ptr[0] = eol + sizeof(im);
   numinodes = 1;
   lseek(fd, eol, SEEK_SET);
   write(fd, &im, sizeof(im));

   //create first inode (root directory)
   eol += sizeof(im);
   inode root;
   root.type = 0; 
   root.size = 2;
   root.data_ptr[0] = eol + sizeof(root);
   root.data_ptr[1] = eol + sizeof(root) + sizeof(MFS_DirEnt_t);
   lseek(fd, eol, SEEK_SET);
   write(fd, &root, sizeof(root));

   //create single root directory and populate (with . and ..)
   MFS_DirEnt_t dr1;
   dr1.name[0] = '.';
   dr1.inum = 0; //roots inode number
   eol += sizeof(root);
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr1, sizeof(dr1));

   MFS_DirEnt_t dr2;
   eol += sizeof(dr1);
   dr2.name[0] = '.';
   dr2.name[1] = '.';
   dr2.inum = dr1.inum; //can't go past this
   lseek(fd, eol, SEEK_SET);
   write(fd, &dr2, sizeof(dr2));

   eol += sizeof(dr2);
   cr.eol = eol;
   lseek(fd, 0, SEEK_SET); 
   write(fd, &cr, sizeof(cr)); 
   printf("done writing everything out");   
}

int main(int argc, char *argv[]) {
   receiving(10021);

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




   fsync(fd); //TODO needed to push data out to disk
   close(fd);
   return 0;
}


