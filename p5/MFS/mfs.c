#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "udp.h"
#include "mfs.h"

#define BUFFERSIZE (4107)

//Message transfer protocol useful defines
#define COMMAND_BYTE (BUFFERSIZE-9)
#define DATA_BLOCK (0)
#define KEY_BYTE (BUFFERSIZE-11)
#define MESSAGE_ID (BUFFERSIZE-10) //1 byteb
#define CMD_INT1 (BUFFERSIZE-8)
#define CMD_INT2 (BUFFERSIZE-4)
#define TIMEOUT (3)

char message[BUFFERSIZE];
char response[BUFFERSIZE];
char test[4097];
struct sockaddr_in saddr;
struct sockaddr_in raddr;
int sd, rc; //socket descriptor, return code
int messageid;

int MFS_Init(char *hostname, int port)
{
    if (DEBUG) printf("MFS_Init called with hostname %s and port %d.\n", hostname, port);

    //INIT VARIABLES
    //messageid = 0; //start the messageid count

    //PRIMATIVE CHECKS
    if (port < 0 || hostname == NULL){
        if (DEBUG) printf("ERROR: MFS_Init received invalid parameters.\n");
        return -1;
    }

    //SET UP SOCKET
    sd = UDP_Open(0);
    if (sd < 0){
        if (DEBUG) printf("UDP_Open in MFS_Init failed.\n");
        return -1;
    }
    rc = UDP_FillSockAddr(&saddr, hostname, port);
    if (rc < 0){
        if (DEBUG) printf("UDP_FillSockAddr in MFS_Init failed.\n");
        return -1;
    }
    fcntl(sd, F_SETFL, O_NONBLOCK); //set to non-blocking

    //SET UP PACKET
    //sprintf(message, "This is a test. #YOLO THIS IS DIFFERENT");
    message[KEY_BYTE] = 'k';
    message[COMMAND_BYTE] = 0;
    message[MESSAGE_ID] = messageid;

    //SEND PACKET
    transmit();
    
    return 0;
}

int MFS_Lookup(int pinum, char *name)
{
    //recieve parent inode number
    //find entry name
    //return inode of name
    //else return -1 (invalid pinum, name does not exist in pinum)

    if (DEBUG) printf("MFS_Lookup called with pinum %d and name %s.\n", pinum, name);

    //PRIMATIVE CHECKS
    if (pinum < 0 || name == NULL || strlen(name) > 60){
        printf("ERROR: MFS_Lookup received invalid parameters.\n");
        return -1;
    }

    //SET UP PACKET
    sprintf(message, name);
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 1;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = pinum;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Lookup received failing packet.");        
        return -1;
    }
    
    return response[CMD_INT2];
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    //return stat info about file inum
    //return 0, else return -1 if inum doesn't exist

      if (DEBUG) printf("MFS_Stat called with inum %d.\n", inum);

    //PRIMATIVE CHECKS
    if (inum < 0 || m == NULL){
        if (DEBUG) printf("ERROR: MFS_LStat received invalid parameters.\n");
        return -1;
    }

    //SET UP PACKET
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 2;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = inum;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Stat received failing packet.");        
        return -1;
    }

    memcpy(m, &response[DATA_BLOCK], 8);

    if (DEBUG) printf("m->type = %d\n", m->type);
    if (DEBUG) printf("m->size = %d\n", m->size);   

    return 0;
}

int MFS_Write(int inum, char *buffer, int block)
{
    //write 4K block offset specified by block
    //return 0 on success, -1 on failure
    //invalid inum, invalid block, not a regular file

    //CRUDE CHECKS
    if (inum < 0 || block < 0 || buffer == NULL)
    {
        if (DEBUG) printf("ERROR: MFS_Write received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    memcpy(message, buffer, 4096);
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 3;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = inum;
    message[CMD_INT2] = block;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Write received failing packet.");        
        return -1;
    }

    return 0;
}

int MFS_Read(int inum, char *buffer, int block)
{
    //reads 4k block specified by block into buffer 
    //from file inum
    //directories return data in format of MFS_DirEnt_t
    //return 0 on success, -1 on failure
    //invalid inum, invalid block
    
    //CRUDE CHECKS
    if (inum < 0 || block < 0 || buffer == NULL)
    {
        if (DEBUG) printf("ERROR: MFS_Read received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    //sprintf(message, "The greatest gift is a passion for reading.");
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 4;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = inum;
    message[CMD_INT2] = block;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Read received failing packet.");        
        return -1;
    }

    memcpy(buffer, response, 4096);    

    return 0;
}

int MFS_Creat(int pinum, int type, char *name)
{
    //make a file of type type 
    //in parent directory specified by pinum of name name
    //return 0 on success, -1 on failure
    //pinum does not exist, name is too long
    //if name already exists, return success

    //CRUDE CHECKS
    if (pinum < 0 ||  name == NULL || strlen(name) > 60)
    {
        if (DEBUG) printf("ERROR: MFS_Creat received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    memcpy(message, name, strlen(name)+1); //include '\0'
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 5;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = pinum;
    message[CMD_INT2] = type;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Creat received failing packet.");        
        return -1;
    }

    return 0;
}

int MFS_Unlink(int pinum, char *name)
{
    //remove file or directory name from directory specified by pinum
    //return 0 on success, -1 on failure
    //pinum does not exist, directory is not empty
    //name not exisiting is not a failure

    //CRUDE CHECKS
    if (pinum < 0 ||  name == NULL || strlen(name) > 60)
    {
        if (DEBUG) printf("ERROR: MFS_Unlink received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    memcpy(message, name, strlen(name)+1); //include '\0'
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 6;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = pinum;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Unlink received failing packet.");        
        return -1;
    }

    return 0;
}

int MFS_Shutdown()
{
    //tells serve to sync and exit(0)

    //SET UP PACKET
    //sprintf(message, "Better to flee death than to feel it's grip.");
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 7;
    message[MESSAGE_ID]   = messageid;

    //SEND PACKET
    transmit();

    messageid = 0;
    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        if (DEBUG) printf("ERROR: MFS_Shutdown received failing packet.");        
        return -1;
    }

    return 0;
}

//SENDS A PACKET OVER THE UDP SOCKET AND VERIFIES THE SERVER RESPONSE
int transmit() //send buffer[], receive response[]
{
   int i;
    time_t tstart, tnow;
    int ackd = 0;
    int timeout = 0;
    int rx = -1;
    int diff;
    while (!ackd)
    {
        tstart = time(NULL);
        //CLEAR RX BUFFER
         for (i = 0;i < BUFFERSIZE;i++) response[i] = 0x00; 
        sendpacket();
        timeout = 0;
        while(!timeout)
        {
            rx = receive();
            tnow = time(NULL);
            if (rx >= 0)// && 0 == verify())
            {
                if ((response[KEY_BYTE] == 'k')&&(response[MESSAGE_ID] == (messageid+1) %255)) {
                //if ((response[KEY_BYTE] == 'k')&&(response[MESSAGE_ID] == messageid)) {
                   if (DEBUG) printf("Server acknowledged request. It's VALID.\n");;
                  messageid = (messageid + 1) % 255; //TODO messing here
                ackd = 1;
                timeout = 1;
                }
            }
            diff = difftime(tnow, tstart);
            if (diff > TIMEOUT)
            {
                if (DEBUG) printf("Client has no received server response, timed out. Resending.\n");
                timeout = 1;
            }
        }//while !timeout
    }//while !ackd
    return 0;
}

//ATTEMPTS TO RECIEVE A PACKET OVER THE UDP SOCKET
int receive()
{   
    rc = UDP_Read(sd, &raddr, response, BUFFERSIZE);
    if (rc < 0)
    {
        return -1;
    }
//    printf("Client : Received message (%d bytes) (%s)\n.", rc, response);

   if (DEBUG) printf("RX: command: %d, key: %c, id: %d, int1: %d, int2: %d\n", response[COMMAND_BYTE], 
 	    response[KEY_BYTE], response[MESSAGE_ID], response[CMD_INT1], response[CMD_INT2]); 
   memcpy(test, response, 4096);
   test[4096] = '\0'; 
   //if (DEBUG) printf("data: %s\n", test);

    return rc;
}

//ATTEMPTS TO SEND A PACKET OVER THE UDP SOCKET
int sendpacket()
{
    rc = UDP_Write(sd, &saddr, message, BUFFERSIZE);
 
    if (DEBUG) printf("TX: command: %d, key: %c, id: %d, int1: %d, int2: %d\n", message[COMMAND_BYTE], 
 	    message[KEY_BYTE], message[MESSAGE_ID], message[CMD_INT1], message[CMD_INT2]); 
   memcpy(test, message, 4096);
   test[4096] = '\0'; 
   if (DEBUG) printf("data: %s\n", test);

   assert(rc > -1);
   return 0;
}

//CHECKS THAT RESPONSE IS VALID FOR THE SENT BUFFER
int verify()
{
//    return 0;
    if( message[COMMAND_BYTE] == response[COMMAND_BYTE] && message[MESSAGE_ID] == response[MESSAGE_ID] 
        && message[KEY_BYTE] == response[KEY_BYTE] && response[CMD_INT1] == 'a' && response[CMD_INT1+1] == 'c'
        && response[CMD_INT1+2] == 'k' && response[CMD_INT1+3] == 'd')
        return 0;
    return -1;
}

