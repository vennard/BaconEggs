#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "udp.h"
#include "mfs.h"

#define BUFFERSIZE (4107)

//Message transfer protocol
#define COMMAND_BYTE (BUFFERSIZE-9)
#define DATA_BLOCK (0)
#define KEY_BYTE (BUFFERSIZE-11)
#define MESSAGE_ID (BUFFERSIZE-10) //1 byteb
#define CMD_INT1 (BUFFERSIZE-8)
#define CMD_INT2 (BUFFERSIZE-4)
#define TIMEOUT (3)

char message[BUFFERSIZE];
char response[BUFFERSIZE];
struct sockaddr_in saddr;
struct sockaddr_in raddr;
int sd, rc; //socket descriptor, return code
int messageid;

int MFS_Init(char *hostname, int port)
{
    printf("MFS_Init called with hostname %s and port %d.\n", hostname, port);

    //INIT VARIABLES
    messageid = 0; //start the messageid count

    //PRIMATIVE CHECKS
    assert (port > -1);
    assert (hostname != NULL);

    //SET UP SOCKET
    sd = UDP_Open(0);
    assert(sd > -1);
    rc = UDP_FillSockAddr(&saddr, hostname, port);
    assert (rc == 0);
    fcntl(sd, F_SETFL, O_NONBLOCK); //set to non-blocking

    //SET UP PACKET
    sprintf(message, "This is a test.");
    //message[DATA_BLOCK] = "This is a test.\n";
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 0;
    message[MESSAGE_ID]   = messageid;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    
    return 0;
}

int MFS_Lookup(int pinum, char *name)
{
    //recieve parent inode number
    //find entry name
    //return inode of name
    //else return -1 (invalid pinum, name does not exist in pinum)

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
        printf("ERROR: MFS_Lookup received failing packet.");        
        return -1;
    }
    return response[CMD_INT2];
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    //return stat info about file inum
    //return 0, else return -1 if inum doesn't exist

    //SET UP PACKET
    //sprintf(message, name);
    message[KEY_BYTE]     = 'k';
    message[COMMAND_BYTE] = 2;
    message[MESSAGE_ID]   = messageid;
    message[CMD_INT1] = inum;

    //SEND PACKET
    transmit();

    //VERIFY PACKET CONTENTS
    if (response[CMD_INT2] < 0)
    {        
        printf("ERROR: MFS_Stat received failing packet.");        
        return -1;
    }
    m->type = response[DATA_BLOCK];
    m->size = response[DATA_BLOCK+4];

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
        printf("ERROR: MFS_Write received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    sprintf(message, buffer);
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
        printf("ERROR: MFS_Write received failing packet.");        
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
        printf("ERROR: MFS_Read received invalid parameters.\n");
        return -1;
    }
   
    //SET UP PACKET
    //sprintf(message, buffer);
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
        printf("ERROR: MFS_Read received failing packet.");        
        return -1;
    }

    //*buffer = 

    return 0;
}

int MFS_Creat(int pinum, int type, char *name)
{
    //make a file of type type 
    //in parent directory specified by pinum of name name
    //return 0 on success, -1 on failure
    //pinum does not exist, name is too long
    //if name already exists, return success
    return 0;
}

int MFS_Unlink(int pinum, char *name)
{
    //remove file or directory name from directory specified by pinum
    //return 0 on success, -1 on failure
    //pinum does not exist, directory is not empty
    //name not exisiting is not a failure
    return 0;
}

int MFS_Shutdown()
{
    //tells serve to sync and exit(0)
    return 0;
}

//SENDS A PACKET OVER THE UDP SOCKET AND VERIFIES THE SERVER RESPONSE
int transmit() //send buffer[], receive response[]
{
    time_t tstart, tnow;
    int ackd = 0;
    int timeout = 0;
    int rx = -1;
    int diff;
    while (!ackd)
    {
        tstart = time(NULL);
        sendpacket();
        timeout = 0;
        while(!timeout)
        {
            rx = receive();
            tnow = time(NULL);
            if (0 == rx && 0 == verify())
            {
                printf("Server acknowledged request. It's response was valid");;
                ackd = 1;
                timeout = 1;
            }
            diff = difftime(tnow, tstart);
            if (diff > TIMEOUT)
            {
                printf("Client has no received server response, timed out. Rsending.");
                timeout = 1;
            }
        }//while !timeout
    }//while !ackd
    messageid = (messageid + 1) % 255; //update, only 1 byte large
    return 0;
}

//ATTEMPTS TO RECIEVE A PACKET OVER THE UDP SOCKET
int receive()
{   
    rc = UDP_Read(sd, &raddr, response, BUFFERSIZE);
    if (rc < 0)
    {
        printf("Client: Received failed..\n");
        return -1;
    }
    printf("Client : Received message (%d bytes) (%s)\n.", rc, response);
    return rc;
}

//ATTEMPTS TO SEND A PACKET OVER THE UDP SOCKET
int sendpacket()
{
    printf("Client : Sending message..\n");
    rc = UDP_Write(sd, &saddr, message, BUFFERSIZE);
    assert(rc > -1);
    printf("Client : Sent message..\n");
    return 0;
}

//CHECKS THAT RESPONSE IS VALID FOR THE SENT BUFFER
int verify()
{
    if(message[COMMAND_BYTE] == response[COMMAND_BYTE] && message[MESSAGE_ID] == response[MESSAGE_ID] 
       && message[KEY_BYTE] == response[KEY_BYTE] && response[CMD_INT1] == 'a' && response[CMD_INT1+1] == 'c'
       && response[CMD_INT1+2] == 'k' && response[CMD_INT1+3] == 'd')
        return 0;
    else return -1;
}
