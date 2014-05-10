#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "udp.h"

#define BUFFER_SIZE (4096)
#define TIMEOUT (5)

char buffer[BUFFER_SIZE];
struct sockaddr_in saddr;
int sd, rc;
int messageid;

//sets up socket
void setupconnection() {
    sd = UDP_Open(0);
    assert(sd > -1);
    rc = UDP_FillSockAddr(&saddr, "best-mumble.cs.wisc.edu", 10021);
    assert(rc == 0);
    messageid = 0; 
    fcntl(sd, F_SETFL, O_NONBLOCK); //set to non-blocking
}

//reads packet from socket 
int receive(){
    if (rc > 0) {
	    struct sockaddr_in raddr;
	    rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
char buffer[BUFFER_SIZE];
//struct sockaddr_in saddr;
//int sd, rc;
//int messageid;
	    printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
        return 0;
    }
    return 1;
}

//sends packet out through socket
int sendpacket(char message[BUFFER_SIZE]){
    printf("CLIENT:: about to send message (%d)\n", rc);
    rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d)\n", rc);
    return 0;
}

//Using first char byte as acknowledge
//transmits data and waits for acknowledge
//CANNOT USE FULL BUFFER
//DATA WILL BE STORED AT THE LAST 3 BYTES OF THE BUFFER
int transmit(char message[BUFFER_SIZE]) {
    time_t tstart,tnow;
    int ackd = 0;
    message[BUFFER_SIZE-3] = messageid;
    message[BUFFER_SIZE-2] = 'k';
    message[BUFFER_SIZE-1] = 'z'; //watch for this key TODO remove
    while (!ackd) {
        tstart = time(NULL);
        rc = UDP_FillSockAddr(&saddr, "best-mumble.cs.wisc.edu", 10021);
        assert(rc == 0);
        sendpacket(message);
        int timeout = 0;
        while (!timeout) {
            int rxd = receive();
            tnow = time(NULL);
            if ((rxd == 0)&&(messageid == (int)buffer[0])&&(buffer[1] == 'a')&&(buffer[2] == 'c')&&(buffer[3] == 'k')) { //got valid ack
                printf("SUCCESS received correct acknowledge!\r\n");
                ackd = 1;
                timeout = 1;
            }
            int diff = difftime(tnow, tstart);
            if (diff > TIMEOUT) {
                timeout = 1;
                printf("Timed out! Resending...\r\n");
            }
        }
    }
    messageid++; //increment AFTER sending message
    return 0;
}

int main(int argc, char *argv[]) {
    setupconnection();
    char sendingthis[BUFFER_SIZE];
    sprintf(sendingthis, "well i guess this does work -- suspiciously slow though");
    transmit(sendingthis);
    sprintf(sendingthis, "trying to send something else !!!");
    transmit(sendingthis);
    //If you send a request that wants data back it will be put into buffer after transmission
    //remember I use the last 3 bytes so those will be unuseable
    return 0;
}


