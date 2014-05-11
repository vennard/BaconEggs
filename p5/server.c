#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

//Message transfer protocol useful defines
#define BUFFERSIZE (4107)
#define COMMAND_BYTE (BUFFERSIZE-9)
#define DATA_BLOCK (0)
#define KEY_BYTE (BUFFERSIZE-11)
#define MESSAGE_ID (BUFFERSIZE-10)
#define CMD_INT1 (BUFFERSIZE-8)
#define CMD_INT2 (BUFFERSIZE-4)

int messagecount;
char buffer[BUFFERSIZE];
int MFS_Lookup_h(int pinum, char *name);
int MFS_Init_h(char *hostname, int port);
int MFS_Stat_h(int inum, MFS_Stat_t *m);
int MFS_Write_h(int inum, char *buffer, int block);
int MFS_Read_h(int inum, char *buffer, int block);
int MFS_Creat_h(int pinum, int type, char *name);
int MFS_Unlink_h(int pinum, char *name);
int MFS_Shutdown_h();

//loop waiting for data to be recieved
void receiving() {
    int sd = UDP_Open(10021);
    assert(sd > -1);
    printf("SERVER: About to enter receiver waiting loop!\r\n");
    messagecount = 0;
    while (1) {
	    struct sockaddr_in s;
	    int rc = UDP_Read(sd, &s, buffer, BUFFERSIZE);
	    if (rc > 0) {
	        printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
            if ((buffer[BUFFERSIZE-3] == messagecount)&&(buffer[BUFFERSIZE-2] == 'k')&&(buffer[BUFFERSIZE-1] == 'z'))  {
                messagecount++;
                //idempotency -- only process messages once - always ack
                printf("SERVER processing unique message (%d bytes)!\r\n",rc);
            }
	         char reply[BUFFERSIZE];
            reply[0] = buffer[BUFFERSIZE-3]; //send ack number back with special code
            reply[1] = 'a';
            reply[2] = 'c';
            reply[3] = 'k';
	         rc = UDP_Write(sd, &s, reply, BUFFERSIZE);
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
   MFS_Lookup_h(0, "..");

   close(fd);
   return 0;
}

//Finds the entry matching name in the parent directory pinum
//returns inode number of name
int MFS_Lookup_h(int pinum, char *name) {
    printf("Called MFS_Lookup with pinum %i and name %s !\r\n",pinum,name);
    getinode(pinum);
    printf("Found inode of size %i and type %i and ptr[0] - %i\r\n",inode_t.size,inode_t.type,inode_t.data_ptrs[0]);
    int i = 0;
    int ptr = inode_t.data_ptrs[0];
    while(inode_t.data_ptrs[i] != 0) { //search directory blocks
        getentry(ptr);
        direntry *dp = &direntry_t; 
        while (dp->inum != -1) {
        printf("Got entry name - %s and inum - %i\r\n",dp->name,dp->inum);
            if (strcmp(dp->name,name) == 0) {
                printf("Found a match! Returning inum of match: %i \r\n",dp->inum);
                return dp->inum;
            }
            ptr += 64;
            getentry(ptr);
            dp = &direntry_t;
        }
        i++;
        ptr = inode_t.data_ptrs[i];
    }
    printf("failed to find a match! returning -1\r\n");
    return -1;
}




