#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include "json.hpp"

class Daemon
{
private:
    struct sockaddr_in server;
    fd_set readFDs; // reading descriptors
    fd_set activeFDs; // active descriptors
    struct timeval tv; // time structure for select()
    int sd; // socket descriptors
    int fd; // auxiliary descriptor
    int nfds; // maximum number of descriptors
    int len; // length of sockaddr_in structure
    int optVal; // setsockopt() option
    bool debugging; // whether the debugging tool is activated
    char* convAddr(struct sockaddr_in); // convert a struct sockaddr_in to a readable IP address
    void processRequest(int); // spawn a child process to handle a client request
    int port; // daemon port
public:
    Daemon(); // default constructor
    void setDebugging(bool); // toggle the debugging tool
    void watch(); // watch the socket for incoming requests from the clients
    bool setup(); // setup the P2P socket
    int getPort(); // get the daemon port
};

#endif