#include "Daemon.hpp"

#define BUFFER_SIZE 1024

// Source: https://stackoverflow.com/a/6039648/5889056
long getFileSize(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


Daemon::Daemon() {
    this->debugging = false;
    this->optVal = 1;
}

/*
 * Toggle the debugging tool
 * @param bool
 */
void Daemon::setDebugging(bool d) {
    this->debugging = d;
}

/*
 * Setup the P2P daemon instance parameters.
 * @return bool
 */
bool Daemon::setup() {
    
    if((this->sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Encountered socket() error at P2P daemon");
        return false;
    } else {
        if(this->debugging)
            std::cout << "P2P socket created successfully." << std::endl;
    }

    setsockopt(this->sd, SOL_SOCKET, SO_REUSEADDR, &this->optVal, sizeof(this->optVal));

    // Prepare data structures
    bzero(&this->server, sizeof(this->server));

    // Initialize server data structure
    this->server.sin_family = AF_INET;
    // this->server.sin_addr.s_addr = htonl(INADDR_ANY);
    // this->server.sin_port = htons(port);
    // this->server.sin_addr.s_addr = 0;
    this->server.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if(::bind(this->sd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1) {
        perror("Encountered bind() error at P2P socket");
        return false;
    } else {
        socklen_t len = sizeof(this->server);
        if(getsockname(this->sd, (struct sockaddr *)&this->server, &len) != -1)
            this->port = ntohs(this->server.sin_port);
        if(this->debugging)
            std::cout << "P2P socket binded successfully." << std::endl << "Daemon port: " << this->port << std::endl;
    }

    // Check if listen() is working
    if(listen(this->sd, 5) == -1) {
        perror("Encountered listen() error at P2P daemon");
        return false;
    }

    // Initialize the set of reading descriptiors
    FD_ZERO(&this->activeFDs); // the set is initially empty
    FD_SET(this->sd, &this->activeFDs); // insert in the set the created socket

    this->tv.tv_sec = 1; // wait up to 1 second
    this->tv.tv_usec = 0;
  
    this->nfds = sd; // maximum value of used descriptors

    return true;
}

/*
 * Continuously watch for client requests over TCP/IP
 */
void Daemon::watch() {

    while(1001 % 77 == 0) {
        // active descriptors (that are currently used)
        bcopy((char *) &this->activeFDs, (char *) &this->readFDs, sizeof(this->readFDs));
        if(select(this->nfds + 1, &this->readFDs, NULL, NULL, &this->tv) < 0) {
	        perror("Encountered error at P2P daemon select(). Stopping P2P server...");
	        return;
	    }
        if(FD_ISSET(this->sd, &this->readFDs)) { // if the server is ready to accept the client
            struct sockaddr_in from; // the client structure
            // prepare the client structure
	        len = sizeof(from);
	        bzero(&from, sizeof(from));
            // a client connected, try accepting the request
	        int client = accept(sd, (struct sockaddr *) &from, (socklen_t*) &this->len);
	        if(client < 0) { // error accepting a client connection
	            perror("Encountered error at P2P daemon accept(). Continuing to the next request...\n");
	            continue;
	        }
            if(this->nfds < client) // increment the descriptors counter
                nfds = client;
            // add the current client descriptor to the set of active descriptors
	        FD_SET(client, &this->activeFDs);
            if(this->debugging)
	            printf("Peer with descriptor %d made a request from from %s.\n", client, this->convAddr(from));
	    }
        // check if any client socket is ready to receive a message
        for(fd = 0; fd <= nfds; fd++) { // check all active sockets
            // if fd is ready
            if(fd != sd && FD_ISSET(fd, &this->readFDs)) {	
                this->processRequest(fd); // process its request
            }
        }
    }

}

void Daemon::processRequest(int fd) {
    // Create a child process for each incoming request
    int pid;
    switch(pid = fork()) {
        case -1: { // error
            perror("Error creating child process to handle peer request");
            FD_CLR(fd, &this->activeFDs);
            close(fd);
            signal(SIGCHLD, SIG_IGN);
            return;
        }
        case 0:  { // child process

            nlohmann::json incomingRequest;
            nlohmann::json response;
            int cpid = getpid(); // child pid
            char request[1024]; // request
            int bytes; // length of request
            
            bytes = read(fd, request, sizeof(request));
            if(bytes < 0) {
                exit(0);
            }
            if(bytes == 0) {
                close(fd);
            }

            incomingRequest = nlohmann::json::parse(request);

            std::string fileHash = (std::string) incomingRequest["file_hash"];
            std::string extension = (std::string) incomingRequest["extension"];
            std::string path = "/tmp/part2part/files/";
            path += fileHash;
            path += ".";
            path += extension;

            if(incomingRequest["action"] == "handshake") {
                
                // Check if the file exists
                if(!(access(path.c_str(), F_OK) == 0)) {
                    response["success"] = false;
                    response["message"] = "Couldn't find the file on my system.";
                } else {
                    response["success"] = true;
                    response["size"] = getFileSize(path);
                }

                std::string responseStr = response.dump();

                char answer[1024]; 
                bzero(answer, 1024);
                strncat(answer, responseStr.c_str(), responseStr.length());

                if(this->debugging)
                    printf("[%d_daemon] Sending file to peer: %s\n", cpid, answer);

                // responseStr[responseStr.length()] = '\0';

                if(write(fd, answer, responseStr.length()) < responseStr.length()){
                    perror("Encountered error at write() in P2P daemon");
                    exit(0);
                }

            } else if(incomingRequest["action"] == "download") {

                // Check if the file exists
                if(!(access(path.c_str(), F_OK) == 0)) {

                    if(write(fd, "\0", 1) < 1){
                        perror("Encountered error at write() in P2P daemon");
                        exit(0);
                    }

                } else {

                    std::ifstream fin(path, std::ifstream::binary);
                    std::vector<char> buffer(BUFFER_SIZE, 0);

                    while(!fin.eof()) {
                        fin.read(buffer.data(), buffer.size());
                        std::streamsize dataSize = fin.gcount();

                        std::string s(buffer.begin(), buffer.begin() + dataSize);

                        if(write(fd, &buffer[0], dataSize) < dataSize){
                            perror("Encountered error at write() in P2P daemon");
                            exit(0);
                        }
                        // usleep(50000);
                        
                    }

                }

            }
            
            exit(1);

        }
        default: { // parent

            FD_CLR(fd, &this->activeFDs); // Remove the descriptor from the active descriptors
            
        }
    }
}

int Daemon::getPort() {
    return this->port;
}

/*
 * Convert client IP address to char*
 * @param struct sockaddr_in address - client address
 * @return char* - client IP
 * Source: https://profs.info.uaic.ro/~computernetworks/files/NetEx/S9/servTcpCSel.c
 */
char* Daemon::convAddr(struct sockaddr_in address) {
    static char str[25];
    char port[7];
    // Client IP address
    strcpy(str, inet_ntoa(address.sin_addr));	
    // Client port
    bzero(port, 7);
    sprintf(port, ":%d", ntohs(address.sin_port));	
    strcat(str, port);
    return str;
}