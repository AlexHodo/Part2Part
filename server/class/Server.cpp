#include "Server.h"
#define PORT 33333
#define MAX_FILE_SIZE 100000000

void sighandler(int sig){
    wait(NULL);
}

Server::Server() {
    this->debugging = false;
    this->optVal = 1;
}

/*
 * Toggle the debugging tool
 * @param bool
 */
void Server::setDebugging(bool d) {
    this->debugging = d;
    this->db.setDebugging(d);
}

/*
 * Setup the server instance parameters.
 * @return bool
 */
bool Server::setup() {
    
    if((this->sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Encountered socket() error.\n");
        return false;
    } else {
        if(this->debugging)
            std::cout << "Socket created successfully." << std::endl;
    }

    setsockopt(this->sd, SOL_SOCKET, SO_REUSEADDR, &this->optVal, sizeof(this->optVal));

    // Prepare data structures
    bzero(&this->server, sizeof(this->server));

    // Initialize server data structure
    this->server.sin_family = AF_INET;
    this->server.sin_addr.s_addr = htonl(INADDR_ANY);
    this->server.sin_port = htons(PORT);

    // Bind the socket
    if(::bind(this->sd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1) {
        perror("Encountered bind() error");
        return false;
    } else {
        if(this->debugging)
            std::cout << "Socket binded successfully." << std::endl;
    }

    // Check if listen() is working
    if(listen(this->sd, 5) == -1) {
        perror("Encountered listen() error");
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
void Server::watch() {

    if(!this->setup()) { // if setup fails
        std::cout << "Server cannot be started. Ending process now." << std::endl;
        return;
    } 
    std::cout << "Waiting for requests at the following port: " << PORT << std::endl;

    while(1001 % 77 == 0) {
        // active descriptors (that are currently used)
        bcopy((char *) &this->activeFDs, (char *) &this->readFDs, sizeof(this->readFDs));
        if(select(this->nfds + 1, &this->readFDs, NULL, NULL, &this->tv) < 0) {
	        perror("Encountered error at select(). Stopping server...");
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
	            perror("Encountered error at accept(). Continuing to the next request...\n");
	            continue;
	        }
            if(this->nfds < client) // increment the descriptors counter
                nfds = client;
            // add the current client descriptor to the set of active descriptors
	        FD_SET(client, &this->activeFDs);
            if(this->debugging)
	            printf("Client with descriptor %d made a request from %s.\n", client, this->convAddr(from));
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

void Server::processRequest(int fd) {
    // Create a child process for each incoming request
    int pid;
    switch(pid = fork()) {
        case -1: { // error
            perror("Error creating child process to handle client request");
            FD_CLR(fd, &this->activeFDs);
            close(fd);
            signal(SIGCHLD, SIG_IGN);
            return;
        }
        case 0:  { // child process

            // Get client IP
            // Source: https://stackoverflow.com/a/20475352/5889056
            struct sockaddr_in addr;
            socklen_t addr_size = sizeof(struct sockaddr_in);
            int res = getpeername(fd, (struct sockaddr *)&addr, &addr_size);
            char *clientip = new char[20];
            strcpy(clientip, inet_ntoa(addr.sin_addr));
            std::string clientips = clientip;

            nlohmann::json incomingRequest;
            nlohmann::json response;
            int cpid = getpid(); // child pid
            char request[1024]; // request
            int bytes; // length of request
            char answer[1024] = " "; // answer for the client
            bytes = read(fd, request, sizeof(request));
            if(bytes < 0) {
                exit(0);
            }
            if(bytes == 0) { // the client disconnected
                close(fd);
            }
            if(this->debugging)
                printf("[%d] Received: %s\n", cpid, request);

            if(!this->db.connect(DB_PATH)) {
                std::cout << "Could not establish a database connection." << std::endl;
                response["success"] = false;
                response["message"] = "We could not process your request due to a database connection error. Please try again later.";
                exit(0);
            } else {
                // if(this->debugging) 
                //    std::cout << "Database connection established successfully." << std::endl;
                
                std::string tmp = request;
                int last = 0;
                for(int i = 0; i < tmp.length(); i++) {
                    if(tmp[i] == '}') {
                        last = i;
                    }
                }
                tmp = tmp.substr(0, last+1);

                incomingRequest = nlohmann::json::parse(tmp.c_str());

                if(incomingRequest["action"] == "register") {
                    std::string username = incomingRequest["username"];
                    std::string password = incomingRequest["password"];

                    bool ok = true;

                    // Validate input once again
                    if(username.length() < 6) {
                        std::cout << "Please enter at least 6 characters." << std::endl;
                        ok = false;
                    } else {
                        for(int i = 0; i < username.length() && ok; i++) {
                            if(! ((username[i] >= 'a' && username[i] <= 'z' ) || 
                                (username[i] >= 'A' && username[i] <= 'Z') || 
                                (username[i] >= '0' && username[i] <= '9')) ) {
                                std::cout << "Please only enter alphanumeric characters." << std::endl;
                                ok = false;
                            }
                        }
                    }
                    if(password.length() < 6) {
                        std::cout << "Please enter at least 6 characters." << std::endl;
                        ok = false;
                    }

                    if(!ok) {
                        response["success"] = false;
                        response["message"] = "Malformed input. Please follow the rules shown by your client.";
                    } else {
                        std::string passwordHash = SHA256(password.c_str());
                        User u;
                        u.username = username;
                        u.password = passwordHash;
                        if(db.createUser(u)) {
                            response["success"] = true;
                            response["message"] = "You've successfully created a new account. You can now log in.";
                        } else {
                            response["success"] = false;
                            std::string tmp = this->db.messageForClient;
                            response["message"] = tmp.c_str();
                        }
                    }
                    
                } else if(incomingRequest["action"] == "login") {

                    std::string username = incomingRequest["username"];
                    std::string password = incomingRequest["password"];

                    bool ok = true;

                    // Validate input once again
                    if(username.length() < 6) {
                        response["message"] = "Username not found.";
                        ok = false;
                    } else {
                        for(int i = 0; i < username.length() && ok; i++) {
                            if(! ((username[i] >= 'a' && username[i] <= 'z' ) || 
                                (username[i] >= 'A' && username[i] <= 'Z') || 
                                (username[i] >= '0' && username[i] <= '9')) ) {
                                response["message"] = "Username not found.";
                                ok = false;
                            }
                        }
                    } 

                    if(!ok) {
                        response["success"] = false;
                    } else {
                        std::string passwordHash = SHA256(password.c_str());
                        
                        User u = this->db.getUser(username);
                        if(u.id == -1) {
                            response["message"] = "Username not found.";
                            response["success"] = false;
                        } else {
                            if(passwordHash != u.password) {
                                response["message"] = "Wrong password.";
                                response["success"] = false;
                            } else {

                                std::string hashPayload = std::to_string(u.id);
                                hashPayload += randStr(256);

                                Session s;
                                s.userId = u.id;
                                s.sessionHash = SHA256(hashPayload.c_str());
                                s.ip = clientips;
                                s.port = std::to_string((int)incomingRequest["daemon_port"]);

                                if(this->db.createSession(s)) {
                                    response["session_hash"] = s.sessionHash;
                                    response["message"] = "You are now logged in.";
                                    response["success"] = true;
                                } else {
                                    response["message"] = "We could not log you in. Please try again later.";
                                    response["success"] = false;
                                }
                                
                            }

                        }

                    }

                } else {

                    std::vector<std::string> protectedActions = {"upload", "search", "download"};
                    if(std::find(protectedActions.begin(), protectedActions.end(), incomingRequest["action"]) != protectedActions.end()) {

                        int uid = this->middleware(incomingRequest["session_hash"]);

                        if(uid == -1) {
                            response["success"] = false;
                            response["message"] = "Please log in again.";
                        } else {

                            if(incomingRequest["action"] == "upload") {

                                if(incomingRequest["size"] == 0) {
                                    response["success"] = false;
                                    response["message"] = "The file you selected is invalid or empty.";
                                } else if(incomingRequest["size"] > MAX_FILE_SIZE) {
                                    response["success"] = false;
                                    response["message"] = "The file is too large. Up to 100MB allowed.";
                                } else {

                                    File f;
                                    f.hash = incomingRequest["hash"];
                                    f.title = incomingRequest["title"];
                                    f.size = incomingRequest["size"];
                                    f.extension = incomingRequest["extension"];
                                    f.uploaderId = uid;


                                    if(this->db.createFile(f)) {
                                        response["success"] = true;
                                        response["message"] = "Upload successful.";
                                    } else {
                                        response["success"] = false;
                                        response["message"] = this->db.messageForClient;
                                    }

                                }

                            } else if(incomingRequest["action"] == "search") {

                                std::string title, extension, username;
                                title = incomingRequest["title"] == "*"? "" : incomingRequest["title"];
                                extension = incomingRequest["extension"] == "*"? "" : incomingRequest["extension"];
                                username = incomingRequest["username"] == "*"? "" : incomingRequest["username"];

                                std::vector<File> files = this->db.searchFiles(title, extension, username);

                                response["success"] = true;
                                response["message"] = "";
                                response["files"] = nlohmann::json::array();

                                int k = 0;
                                for(auto f : files) {
                                    response["files"][k++] = {
                                        {"id", std::to_string(f.id)},
                                        {"title", f.title},
                                        {"size", std::to_string(f.size)},
                                        {"date", f.createdAtString},
                                        {"username", f.username},
                                        {"extension", f.extension}
                                    };
                                }

                                if(!k) {
                                    response["message"] = "No results.";
                                    response["success"] = false;
                                }

                            } else if(incomingRequest["action"] == "download") {

                                int fileId = incomingRequest["file_id"];
                                File f = this->db.getFile(fileId);

                                if(f.id == -1) {
                                    response["success"] = false;
                                    response["message"] = "The requested file could not be found.";
                                } else {
                                    response["success"] = true;
                                    response["ip"] = f.lastLogin.ip;
                                    response["port"] = f.lastLogin.port;
                                    response["file_hash"] = f.hash;
                                    response["extension"] = f.extension;
                                }

                            }

                        }

                    } else {

                        response["success"] = false;
                        response["message"] = "We couldn't process your request. Please try again later.";

                    }
                    
                }

                std::string tmp2 = response.dump();
                tmp2[tmp2.length()] = '\0';
                bzero(answer, 1024);
                strcat(answer, tmp2.c_str());

            }

            if(this->debugging)
                printf("[%d] Sending answer to client: %s\n", cpid, answer);

            if(write(fd, answer, strlen(answer)+1) < 0){
                perror("Encountered error at write()");
                exit(0);
            }
            
            exit(1);

        }
        default: { // parent

            FD_CLR(fd, &this->activeFDs); // Remove the descriptor from the active descriptors
            
        }
    }
}

/*
 * Convert client IP address to char*
 * @param struct sockaddr_in address - client address
 * @return char* - client IP
 * Source: https://profs.info.uaic.ro/~computernetworks/files/NetEx/S9/servTcpCSel.c
 */
char* Server::convAddr(struct sockaddr_in address) {
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

/*
 * Generate a random string.
 * @param const int
 * @return std::string
 * Source: https://stackoverflow.com/a/440240/5889056
 */
std::string randStr(const int len) {
    srand (time (0));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}

int Server::middleware(std::string sessionHash) {
    Session s = this->db.getSession(sessionHash); 
    return s.userId; // either -1 or valid user id
}