#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)
//local variables
int portnum, fd;
char *filesystem;
int messagecount;
char buffer[BUFFER_SIZE];
MFS_Stat_t stat_t;

//local functions
int MFS_Lookup_h(int pinum, char *name);
int MFS_Init_h();
int MFS_Stat_h(int inum);
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
	    int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
	    if (rc > 0) {
	        printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
            if ((buffer[BUFFER_SIZE-3] == messagecount)&&(buffer[BUFFER_SIZE-2] == 'k')&&(buffer[BUFFER_SIZE-1] == 'z'))  {
                messagecount++;
                //idempotency -- only process messages once - always ack
                //processcommand();
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
   //MFS_Lookup_h(0, "..");
   //MFS_Stat_h(0);
   char buf[60];
   sprintf(buf, "new data block info");
   if (-1 == MFS_Write_h(1, buf, 0)) printf("Failed!\r\n"); //should fail, needs to be called on created inode

   close(fd);
   return 0;
}

//takes order from receiving, processes it and puts
//necessary data in buffer to be sent back out as ack
//returns 0 on success, -1 on failure
int processcommand(int cmd) {
    switch (cmd) {
        case 0: //MFS_Init
            break;
        case 1: //MFS_Lookup
            break;
        case 2: //MFS_Stat
            break;
        case 3: //MFS_Write
            break;
        case 4: //MFS_Read
            break;
        case 5: //MFS_Creat
            break;
        case 6: //MFS_Unlink
            break;
        case 7: //MFS_Shutdown
            break;
        default:
            return -1;
            break;
    }
    //TODO save relavent data in buffer to be sent back out
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


//Takes host name and port number 
int MFS_Init_h() {
    printf("Called MFS_Init handler... sending back acknowledge\r\n");
    return 0;
}

//reuturns some info about file specificed by inum
//reuturns 0 on success, -1 on failure
//saves data in stat
int MFS_Stat_h(int inum) {
    printf("Getting stat data about file... ");
    if (getinode(inum) == -1) return -1; 
    printf(" got inode: size: %i, type %i\r\n",inode_t.size,inode_t.type);
    stat_t.size = inode_t.size;
    stat_t.type = inode_t.type;
    return 0;
}

//write handler, writes buf to block
int MFS_Write_h(int inum, char *buf, int block) {
    int eol, blkptr, inodeptr, imapptr, imap, inode;
    printf("Called MFS_Write...");
    if (getinode(inum) == -1) return -1; //fail on invalid inum
    if (inode_t.type != 1) return -1; //fail on not regular file
    if ((block < 0)||(block > 13)) return -1; //fail on invalid block
    //get end of log
    eol = geteol();
    printf(" got eol = %i\r\n",eol);
    //write new data block
    blkptr = eol; 
    eol = writeblock(eol, buf, 4096); 
    //write new inode after updating with new block ptr
    inodeptr = eol;
    inode_t.data_ptrs[block] = blkptr; 
    eol = writeblock(eol, (char*)&inode_t, 64);
    printf("new inode saved at %i and new imap block saved at %i\r\n",inodeptr,eol);
    //write new imap piece after updating with new ptr
    imap = inum / 16;
    inode = inum % 16;
    imapptr = eol;
    char *ptr = readblock(4 + (imap*4), 4); //get old imap loc
    int offset = (int)*ptr;
    ptr = readblock(offset, 64); //read old imap
    int imaps[16];
    int i;
    for(i = 0;i < 16;i++) {
        imaps[i] = (int)*ptr;
        ptr += 4;
    } //TODO left off here
    imaps[inode] = inodeptr; //set new ptr 
    imapptr = eol;
    eol = writeblock(eol, (char*)imaps, 64); //write out 
    //then update checkregion ptr and eol
    writeblock(4+(imap*4), (char*)&imapptr, 4);
    writeblock(0, (char*)&eol, 4);
    return 0;
}

int MFS_Read_h(int inum, char *buffer, int block) {
    printf("Called MFS_Read...");
      
   return 0;
}
