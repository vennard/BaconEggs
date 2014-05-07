#include <stdio.h>
#include "udp.h"

#define BUFFER_SIZE (4096)
char buffer[BUFFER_SIZE];
struct sockaddr_in saddr;
int sd, rc;
int messageid;

//sets up socket
void setupconnection() {
    sd = UDP_Open(0);
    assert(sd > -1);
    rc = UDP_FillSockAddr(&saddr, "best-mumble.cs.wisc.edu", 10001);
    assert(rc == 0);
    
}

//reads packet from socket 
int receive(){
    if (rc > 0) {
	 struct sockaddr_in raddr;
	 rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
	 printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
    }
    return 0;
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
int transmit() {
    
    return 0;
}

int main(int argc, char *argv[]) {
    setupconnection();
    char message[BUFFER_SIZE];
    sprintf(message, "hello world");
    sendpacket(message);
    receive();
    /*
    printf("CLIENT:: about to send message (%d)\n", rc);
    char message[BUFFER_SIZE];
    sprintf(message, "hello world");

    rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d)\n", rc);
    if (rc > 0) {
	 struct sockaddr_in raddr;
	 int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
	 printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
    }
    */

    return 0;
}


