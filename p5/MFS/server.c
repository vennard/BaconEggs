#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4107)
#define COMMAND_BYTE (BUFFER_SIZE-9)
#define DATA_BLOCK (0)
#define KEY_BYTE (BUFFER_SIZE-11)
#define MESSAGE_ID (BUFFER_SIZE-10)
#define CMD_INT1 (BUFFER_SIZE-8)
#define CMD_INT2 (BUFFER_SIZE-4)
#define TIMEOUT (3)

//local variables
int portnum, fd, rc, sd;
struct sockaddr_in s;
char *filesystem;
int messagecount;
char buffer[BUFFER_SIZE];
char reply[BUFFER_SIZE];
char data[4097];
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
int processcommand(int cmd);

//loop waiting for data to be recieved
void receiving(int portnum) {
    sd = UDP_Open(portnum);
    assert(sd > -1);
    if (DEBUG) printf("SERVER: About to enter receiver waiting loop!\r\n");
    while (1) {
	    rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
	    if (rc > 0) {
	        if(DEBUG) printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
           if (DEBUG) printf("Recieved command %d, messageid %d, key value %c, command int one %d, command int two %d.\n",
          buffer[COMMAND_BYTE], buffer[MESSAGE_ID], buffer[KEY_BYTE], buffer[CMD_INT1], buffer[CMD_INT2]);
            if ((buffer[KEY_BYTE] == 'k')&&(buffer[MESSAGE_ID] == messagecount)) {
                messagecount = (messagecount + 1) % 255; //match with client roll over 
                //idempotency -- only process messages once - always ack
                memcpy(data, buffer, 4096);
                data[4096] = '\0';
                processcommand(buffer[COMMAND_BYTE]);
                if (DEBUG) printf("SERVER processing unique message (%d bytes)!\r\n",rc);
            } else { //send dumb ack
               reply[COMMAND_BYTE] = buffer[COMMAND_BYTE];   
               reply[MESSAGE_ID] = messagecount; //TODO messing here 
               //reply[MESSAGE_ID] = buffer[MESSAGE_ID];
               reply[KEY_BYTE] = 'k';
               //memcpy(&reply[CMD_INT1], ackd, 4);
               reply[CMD_INT2] = 0; //return val
	            rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            }
         //clear buffers
         int i;
         for(i = 0;i < BUFFER_SIZE;i++) {
             buffer[i] = 0x00;
             reply[i] = 0x00;
             if (i < 4097) data[i] = 0x00;
            }
	    }
    }
}

//TODO increase directory sizes correctly
int main(int argc, char *argv[]) {
   messagecount = 0; //initialize ack count

   //check and save off input args
   if (argc != 3) {
      if (DEBUG)printf("Incorrect command line arguments: needs server [portnum] [filesystem] \r\n");
      return 1;
   } 
   portnum = atoi(argv[1]);
   filesystem = argv[2];

   
   //try and open filesystem -- if it doesn't exist create a new one
   fd = open(filesystem, O_RDWR, 666);
   if (fd >= 0) {
      if (DEBUG) printf("Opened existing filesystem: %s!\r\n",filesystem);
   }
   if (fd < 0) startfs(filesystem);

   //change permissions on file
   //char str[64];
   //sprintf(str, "chmod a+wr %s", filesystem);
   //system(str);
   

   receiving(portnum);
   return 0;
}

//takes order from receiving, processes it and puts
//necessary data in buffer to be sent back out as ack
//returns 0 on success, -1 on failure
int processcommand(int cmd) {
    int retval, i;
    char bf[5];
    char clear = 0x00;
    sprintf(bf,"ackd");
    switch (cmd) {
        case 0: //MFS_Init
            MFS_Init_h();
            break;
        case 1: //MFS_Lookup
            retval = MFS_Lookup_h(buffer[CMD_INT1], data);
            if (retval == -1) {
                for(i = 0;i < 4096;i++)reply[i] = clear;  
                reply[KEY_BYTE] = 'k';
                reply[MESSAGE_ID] = messagecount;
                reply[COMMAND_BYTE] = 1;
                reply[CMD_INT1] = -1;
                reply[CMD_INT2] = -1;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	             rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            }
            break;
        case 2: //MFS_Stat
            retval = MFS_Stat_h(buffer[CMD_INT1]);
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            //send response
            memcpy(reply, &stat_t, sizeof(stat_t));
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = 2;
            //memcpy(&reply[CMD_INT1],bf,4);  
            reply[CMD_INT2] = retval;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            break;
        case 3: //MFS_Write
            retval = MFS_Write_h(buffer[CMD_INT1],data,buffer[CMD_INT2]);
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            //send response
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = 3;
            //memcpy(&reply[CMD_INT1],bf,4);  
            reply[CMD_INT2] = retval;
                if (DEBUG) printf("RESPONSE: CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            break;
        case 4: //MFS_Read
            retval = MFS_Read_h(buffer[CMD_INT1],rbuf,buffer[CMD_INT2]);
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            memcpy(reply,rbuf,4096);
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = 4;
            //memcpy(&reply[CMD_INT1],bf,4);  
            reply[CMD_INT2] = retval;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            break;
        case 5: //MFS_Creat
            retval = MFS_Creat_h(buffer[CMD_INT1],buffer[CMD_INT2],data);
            if (DEBUG)printf("Calling MFS_Creat handler!\r\n");
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = 5;
            //memcpy(&reply[CMD_INT1],bf,4);  
          //  sprintf(&reply[CMD_INT1],bf);
            reply[CMD_INT2] = retval;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            break;
        case 6: //MFS_Unlink
            retval = MFS_Unlink_h(buffer[CMD_INT1],buffer);
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            reply[CMD_INT2] = retval;
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = 6;
            reply[CMD_INT2] = retval;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            break;
        case 7: //MFS_Shutdown
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            MFS_Shutdown_h();
            break;
        default: //send failed response
            for(i = 0;i < 4096;i++)reply[i] = clear;  
            reply[KEY_BYTE] = 'k';
            reply[MESSAGE_ID] = messagecount;
            reply[COMMAND_BYTE] = -1;
            reply[CMD_INT2] = -1;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	         rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
            return -1;
            break;
    }
    return 0;
}


//Finds the entry matching name in the parent directory pinum
//returns inode number of name
int MFS_Lookup_h(int pinum, char *name) {
    if (DEBUG) printf("\r\n-------------- MFS_Lookup(%i, %s): ",pinum,name);
    getinode(pinum);
    int i = 0;
    int ptr = inode_t.data_ptrs[0];
    while(inode_t.data_ptrs[i] != 0) { //search directory blocks
        getentry(ptr);
        direntry *dp = &direntry_t; 
        while (dp->inum != -1) {
            if (strcmp(dp->name,name) == 0) {
                if (DEBUG) printf("Found entry! return inum %i \r\n",dp->inum);
                //Send reply
                reply[KEY_BYTE] = 'k';
                reply[MESSAGE_ID] = messagecount;
                reply[COMMAND_BYTE] = 1;
                reply[CMD_INT2] = dp->inum; //
                reply[CMD_INT1] = 0;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	             rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
                return dp->inum;
            }
            ptr += 64;
            getentry(ptr);
            dp = &direntry_t;
        }
        i++;
        ptr = inode_t.data_ptrs[i];
    }
    if (DEBUG) printf("failed to find a match! return -1\r\n");
    return -1;
}


//Takes host name and port number 
int MFS_Init_h() {
    if (DEBUG) printf("\r\n---------------MFS_Init: called!\r\n");
    reply[COMMAND_BYTE] = buffer[COMMAND_BYTE];   
    reply[MESSAGE_ID] = messagecount;
    reply[KEY_BYTE] = 'k';
    reply[CMD_INT2] = 0; //return val
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	 rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
    return 0;
}

//reuturns some info about file specificed by inum
//reuturns 0 on success, -1 on failure
//saves data in stat
int MFS_Stat_h(int inum) {
    if (DEBUG) printf("\r\n-------------MFS_Stat(%i): ",inum);
    if (getinode(inum) == -1) return -1; 
    if (DEBUG) printf("found inode size: %i, type %i\r\n",inode_t.size,inode_t.type);
    stat_t.size = inode_t.size;
    stat_t.type = inode_t.type;
    return 0;
}

int MFS_Write_h(int inum, char *buf, int block) {
   if (DEBUG) printf("MFS_Write(%i, %s, %i): ",inum,buf,block);
   int inodeptr = getinode(inum);
   if (inodeptr == -1) return -1; //fail on invalid inum
   if (inode_t.type != 1) return -1; //fail on not regular file
   if ((block < 0)||(block > 13)) return -1; //fail on invalid block
   int eol = geteol();
   inode_t.data_ptrs[block] = eol;
   eol = writeblock(eol, buf, 4096);

   //SIZE THINGS
   int blockcount = 0;
   int lowblock = -1;
   int highblock = -1;
   int set = 0;
   int i;
   for (i = 0; i < 14; i++){
      if(inode_t.data_ptrs[i] != 0){
         if(set == 0){
            lowblock = i;
            set = 1;
         }
         blockcount++;
         highblock = i;
      }
   }
   //(blockcount*4096) ((highblock+1-lowblock)*4096)
   inode_t.size = ((highblock+1-lowblock)*4096);

   writeblock(inodeptr, &inode_t, 64);
   callfsync();
   if (DEBUG) printf("saved new block to %i and increased inode size to %i\r\n",inode_t.data_ptrs[block],inode_t.size);
   return 0;
}

/*
int MFS_Write_h(int inum, char *buf, int block) {
   if (DEBUG) printf("\r\n--------------MFS_Write(%i, %s, %i): ",inum,buf,block);
   int inodeptr = getinode(inum);
   if (inodeptr == -1) return -1; //fail on invalid inum
   if (inode_t.type != 1) return -1; //fail on not regular file
   if ((block < 0)||(block > 13)) return -1; //fail on invalid block
   int eol = geteol();
   inode_t.data_ptrs[block] = eol;
   eol = writeblock(eol, buf, 4096); 
   inode_t.size += 4096;
   writeblock(inodeptr, &inode_t, 64);
   callfsync();
   if (DEBUG) printf("saved new block to %i and increased inode size to %i\r\n",inode_t.data_ptrs[block],inode_t.size);
   return 0;
}
*/

//reads data from inode with inum at block
//saves data at buffer
int MFS_Read_h(int inum, char *buffer, int block) {
    if (DEBUG) printf("\r\n------------MFS_Read(%i, buffer, %i): ",inum,block);
    if (getinode(inum) == -1) return -1; //fails on invalid inum
    if ((block < 0)||(block > 13)||(inode_t.data_ptrs[block] == 0)) return -1; //fail on invalid block
    buffer = readblock(inode_t.data_ptrs[block], 4096);
    if (DEBUG) printf("%s\r\n",buffer);
    return 0;
}

//creates new file or directory in the parent directory pinum
int MFS_Creat_h(int pinum, int type, char *name) {
    if (DEBUG) printf("\r\n-----------------MFS_Creat(%i, %i, %s): ",pinum,type,name);    
    int inodeptr = getinode(pinum);
    if (inodeptr == -1) return -1; //fail on bad pinum
    if (DEBUG) printf("parent inode - type: %i, size: %i, ptr[0]: %i.   ",inode_t.type,inode_t.size,inode_t.data_ptrs[0]);
    if (inode_t.type != 0) return -1; //inum not directory
    if ((type < 0)||(type > 1)) return -1; //invalid type
    if (strlen(name) > 60) return -1; //name too long
    int i = 0; 
    int done = 0;
    //search through inode ptrs for free entry
    while (!done) {
        if(i > 13) return -1; //directory full
        //Check to see if ptr is empty, if so check if its the last block if not create new block of directory points
        if (inode_t.data_ptrs[i] == 0) {
            //create new directory block      
            int j;
            int teol = geteol();
            MFS_DirEnt_t tentry[64];
            for (j = 0;j < 64;j++)tentry[j].inum = -1;
            inode_t.data_ptrs[i] = teol; //update pointer
            inode_t.size += 4096;
            writeblock(inodeptr, &inode_t, 64); //write out new inode 
            teol = writeblock(teol, tentry, 4096); //write new dir block
            seteol(teol); //set new eol
        }
        //search through data blocks for free location
        int check = creatdirentry(inode_t.data_ptrs[i], name);
        if (check == -1) i++; //block full
        if (check == 0) return 0; //found matching name, success
        if (check > 0) { //check is ptr to loc to save new inum
            //create new inode
            inode temp;
            temp.type = type;
            //find free inode number
            int newinum = nextinum();
            if (DEBUG) printf("new inum: %i    ",newinum);
            writeblock(check, &newinum, 4); //write new inum to directory entry
            if (newinum == -1) return -1;
            //write new inode
            int inodeptr;
            int eol = geteol();
            if (type == 0) {
                temp.size = 4096; //dir start with allocated block
                //create base entries . and ..
                temp.data_ptrs[0] = eol;
                MFS_DirEnt_t block[64];
                sprintf(block[0].name, "..");
                block[0].inum = pinum;
                sprintf(block[1].name, ".");
                block[1].inum = newinum;
                //Fill in the remaing inums to -1
                int i;
                for (i = 2;i < 64;i++) block[i].inum = -1;
                eol = writeblock(eol, block, 4096);
                if (DEBUG) printf("new dirblock saved at %i.\r\n",temp.data_ptrs[0]); 
            }
            if (type == 1) {
             temp.size = 0; //files start empty
            }
            //write new inode
            inodeptr = eol;
            eol = writeblock(eol, &temp, 64);
            void *ptr;
            inode *chk;
            ptr = readblock(inodeptr, 64);
            chk = (inode*) ptr;
            if (DEBUG) printf("Read back inode at %i: size - %i, type %i, ptr[0] - %i\r\n",inodeptr, chk->size, chk->type, chk->data_ptrs[0]);
            //read old imap region
            int *imap;
            void *p;
            int imap_loc = 4 + (4 * (newinum / 16));
            p = readblock(imap_loc, 4);
            int *tpt = (int*)p;
            int x = *tpt;
            int imapptr = x;
            if (imapptr < 1) return -1;
            p = readblock(imapptr, 64);
            imap = (int *)p;
            imap[newinum % 16] = inodeptr;
            //write new imap
            int newimap = eol;
            eol = writeblock(eol, imap, 64);
            //update check region (eol and ptr)
            writeblock(4 + (4*(newinum /16)), &newimap, 4);
            writeblock(0, &eol, 4);
            if (DEBUG) printf("Finishing create: newimap region pointer %i in checkregion location %i, eol is updated to %i\r\n",newimap,4+(4*(newinum/16)),eol);
            callfsync();
            return 0;
        }
    }
    return -1; //must be full directory or something weird
}

//removes file or directory from pinum directory
int MFS_Unlink_h(int pinum, char *name) {
   if (DEBUG) printf("\r\n--------------MFS_Unlink: ");
   void *ptr;
   MFS_DirEnt_t *entry;
   int pinum_ptr = getinode(pinum);
   if (pinum_ptr == -1) return -1; //fail on invalid pinum
   //Look through all entries for name
   int i = 0;
   while (inode_t.data_ptrs[i] != 0) {
      ptr = readblock(inode_t.data_ptrs[i], 4096);
      entry = (MFS_DirEnt_t *)ptr;
      int k = 0;
      while(entry[k].inum != -1) { 
         if(strcmp(entry[k].name, name) == 0) {
            if (DEBUG) printf("Found entry to unlink: %s %i",entry[k].name,entry[k].inum);
            //check to see if it is a directory and if its empty
            if (getinode(entry[k].inum) == -1) return -1;
            if (inode_t.type == 0) { //it is a directory
               ptr = readblock(inode_t.data_ptrs[0], 64*3);
               entry = (MFS_DirEnt_t *)ptr;
               if (entry[2].inum != -1) {
                  if (DEBUG) printf(" Error directory is not empty!\r\n");
                  return -1;
               }
            }
            //unlink file or directory
            entry[k].inum = -1;
            sprintf(entry[k].name," ");
            int j = 0;
            for (j = k;j < 63;j++) entry[j] = entry[j+1]; //shift down 
            entry[64].inum = -1;
            ptr = readblock(pinum_ptr, 64);
            inode *ipt = (inode*) ptr;
            writeblock(ipt->data_ptrs[i], entry, 4096);
            if (DEBUG) printf("removed entry %i and rewrote the block at %i\r\n",k,ipt->data_ptrs[i]);
            callfsync();  
            return 0;
         } 
         k++;
      }
     i++; 
   }
   return 0;
}

int MFS_Shutdown_h() {
   if (DEBUG) printf("\r\n-------------MFS_Shutdown: exiting...\r\n");
   reply[KEY_BYTE] = 'k';
   reply[MESSAGE_ID] = messagecount;
   reply[COMMAND_BYTE] = 7;
   char bf[4];
   sprintf(bf,"ackd");
   reply[CMD_INT2] = 0;
                if (DEBUG) printf("RESPONSE: MSG - %s CMD_INT2 - %i KEY - %c MESSAGEID - %i CMDBYTE - %i CMDINT1  - %i\r\n",reply,reply[CMD_INT2],reply[KEY_BYTE],reply[MESSAGE_ID],reply[COMMAND_BYTE],reply[CMD_INT1]);
	rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
   shutdownfs();
   return 0;
}
