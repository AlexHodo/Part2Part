#ifndef SERVER_H
#define SERVER_H

#include "Database.cpp"
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

class Server
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
    bool setup(); // setup the client-server socket
    void processRequest(int); // spawn a child process to handle a client request
    int middleware(std::string); // validate session hash (user id = -1 if invalid)
    Database db;
public:
    Server(); // default constructor
    void setDebugging(bool); // toggle the debugging tool
    void watch(); // watch the socket for incoming requests from the clients
};

std::string randStr(const int len);

#endif