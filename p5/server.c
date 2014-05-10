#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
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
   if (fd < 0) startfs(filesystem);

   printf("Starting testing...\r\n");
   MFS_Lookup(0, "..");

   close(fd);
   return 0;
}

//Finds the entry matching name in the parent directory pinum
//returns inode number of name
int MFS_Lookup(int pinum, char *name) {
    printf("Called MFS_Lookup with pinum %i and name %s !\r\n",pinum,name);
    inode *dir = getinode(pinum);
    printf("Found inode of size %i and type %i\r\n",dir->size,dir->type);
    int i = 0;
    int ptr = dir->data_ptrs[0];
    while(dir->data_ptrs[i] != 0) { //search directory blocks
        direntry *dp = getentry(ptr);
        while (dp->inum != -1) {
            if (strcmp(dp->name,name) == 0) {
                printf("Found a match! Returning inum of match: %i\r\n",dp->inum);
                return dp->inum;
            }
            ptr += 64;
            dp = getentry(ptr);
        }
        i++;
        ptr = dir->data_ptrs[i];
    }
    printf("failed to find a match! returning -1\r\n");
    return -1;
}


