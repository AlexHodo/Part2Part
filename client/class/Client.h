#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <stdarg.h>
#include <signal.h>
#include <fstream>
#include <streambuf>
#include <string.h>
#include "Base64.hpp"
#include "tabulate.hpp"
#include "SHA256.cpp"
#include "json.hpp"
#include "Daemon.cpp"
#include "Base64.hpp"

using namespace tabulate;

class Client
{
private:
    struct sockaddr_in server; // the server data structure required by the TCP/IP socket
    int sd; // the server-client socket descriptor
    bool debugging; // whether the debugging tools is activated
    bool connectedToServer; // whether a socket connection with the server has succesfully been established
    bool connect(); // connect to the server
    bool makeServerRequest(std::string); // make a request to the server
    bool setUpTransferDaemon(); // set up the P2P transfer daemon
    nlohmann::json response; // last server response
    std::string sessionHash; // session identifier
    bool transferDaemonRunning; // whether the transfer daemon is listening for peer requests
    Daemon daemon; // P2P file transfer daemon instance
    bool isLoggedIn();
    bool downloadFile(std::string, std::string, std::string, std::string); // download a file from a peer
public:
    Client(); // default class constructor
    void setServer(struct sockaddr_in); // set the server socket data structure
    void listen(); // listen to user commands
    void setDebugging(bool); // toggle the debugging tool
};

#endif