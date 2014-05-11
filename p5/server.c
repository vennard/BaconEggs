#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)
#define DEBUG (0)
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

//TODO increase directory sizes correctly
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
   if (MFS_Creat_h(0,0,"newdir") != 0) printf("Error with MFS_Creat_h\r\n");
   if (MFS_Creat_h(0,1,"newfile") != 0) printf("Error with MFS_Creat_h\r\n");
   
   MFS_Lookup_h(0, "newfile");
   MFS_Stat_h(0);
   MFS_Stat_h(1);
   MFS_Stat_h(2);
   char out[4096];
   sprintf(out,"testing, once apon a time I could only try and type things that made sense cuz id been programmign too long");
   if (MFS_Write_h(2, out, 0) == -1) printf("Error with MFS_write\r\n");
   if (MFS_Read_h(2, rbuf, 0) == -1) printf("error with MFS_read\r\n");
   if (MFS_Unlink_h(0, "newdir") == -1) printf("Error with MFS_unlink\r\n");
   if (MFS_Creat_h(0,1,"secondfile") != 0) printf("Error with MFS_Creat_h\r\n");
   if (MFS_Creat_h(0,0,"secondfolder") != 0) printf("Error with MFS_Creat_h\r\n");

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
    return 0;
}


//Finds the entry matching name in the parent directory pinum
//returns inode number of name
int MFS_Lookup_h(int pinum, char *name) {
    if (DEBUG) printf("MFS_Lookup(%i, %s): ",pinum,name);
    getinode(pinum);
    int i = 0;
    int ptr = inode_t.data_ptrs[0];
    while(inode_t.data_ptrs[i] != 0) { //search directory blocks
        getentry(ptr);
        direntry *dp = &direntry_t; 
        while (dp->inum != -1) {
            if (strcmp(dp->name,name) == 0) {
                if (DEBUG) printf("Found entry! return inum %i \r\n",dp->inum);
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
    if (DEBUG) printf("MFS_Init: called!\r\n");
    //TODO send ack
    return 0;
}

//reuturns some info about file specificed by inum
//reuturns 0 on success, -1 on failure
//saves data in stat
int MFS_Stat_h(int inum) {
    if (DEBUG) printf("MFS_Stat(%i): ",inum);
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
   int blkptr = eol;
   eol = writeblock(eol, buf, 4096); 
   inode_t.data_ptrs[block] = blkptr;
   inode_t.size += 4096;
   writeblock(inodeptr, &inode_t, 64);
   callfsync();
   if (DEBUG) printf("saved new block to %i and increased inode size to %i\r\n",blkptr,inode_t.size);
   return 0;
}

//reads data from inode with inum at block
//saves data at buffer
int MFS_Read_h(int inum, char *buffer, int block) {
    if (DEBUG) printf("MFS_Read(%i, buffer, %i): ",inum,block);
    if (getinode(inum) == -1) return -1; //fails on invalid inum
    if ((block < 0)||(block > 13)||(inode_t.data_ptrs[block] == 0)) return -1; //fail on invalid block
    buffer = readblock(inode_t.data_ptrs[block], 4096);
    if (DEBUG) printf("%s\r\n",buffer);
    return 0;
}

//creates new file or directory in the parent directory pinum
int MFS_Creat_h(int pinum, int type, char *name) {
    if (DEBUG) printf("MFS_Creat(%i, %i, %s): ",pinum,type,name);    
    if (getinode(pinum) == -1) return -1; //fail on bad pinum
    if (DEBUG) printf("parent inode - type: %i, size: %i, ptr[0]: %i.   ",inode_t.type,inode_t.size,inode_t.data_ptrs[0]);
    if (inode_t.type != 0) return -1; //inum not directory
    if ((type < 0)||(type > 1)) return -1; //invalid type
    if (strlen(name) > 60) return -1; //name too long
    int i = 0; 
    //search through inode ptrs for free entry
    while (inode_t.data_ptrs[i] != 0) {
        //search through data blocks for free location
        int check = creatdirentry(inode_t.data_ptrs[i], name);
        if (check == -1) {
            i++; //block full, check next block
            //TODO increment folder size here?
        }
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
            if (type == 1) temp.size = 0; //files start empty
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
   if (DEBUG) printf("MFS_Unlink: ");
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
   if (DEBUG) printf("MFS_Shutdown: exiting...\r\n");
   //TODO send response first
   shutdownfs();
   return 0;
}
