#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include "class/Client.cpp"
#include <arpa/inet.h>

extern int errno;

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf ("Syntax: %s <server_address> <port>\n", argv[0]);
        return 0;
    }
    int port = atoi(argv[2]);
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]); // server IP address
    server.sin_port = htons(port); // server connection port

    Client c;
    c.setDebugging(false);
    c.setServer(server);

    c.listen();
}