#include "Client.h"

Client::Client() {
    this->debugging = false;
    this->connectedToServer = false;
    this->transferDaemonRunning = false;
    this->sessionHash = "0000";
}

bool Client::isLoggedIn() {
    return this->sessionHash != "0000"; 
}

void Client::setDebugging(bool d) {
    this->debugging = d;
}

bool Client::connect() {
    if((this->sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Encountered error at socket().\n");
        return false;
    }
    if(::connect(this->sd, (struct sockaddr *) &this->server, sizeof(struct sockaddr)) == -1) {
       perror ("Encountered error at connect().\n");
       return false;
    }
    return true;
}

void Client::listen() {
    while(true) {
        nlohmann::json request;
        std::cout << ">>> "; 
        std::string action;
        std::cin >> action;

        if(!this->isLoggedIn() && (action == "upload" || action == "search" || action == "download" || action == "logout")) {
            std::cout << "### You must log in before running this command." << std::endl;
            continue;
        }

        if(action == "help") {
            std::cout << "### Available commands:" << std::endl;
            std::cout << "login \t Log into your account." << std::endl;
            std::cout << "register Create a new account." << std::endl;
            std::cout << "### Once you're logged in, you can also run:" << std::endl;
            std::cout << "upload \t Upload a file." << std::endl;
            std::cout << "search \t Search for files." << std::endl;
            std::cout << "download Download a file." << std::endl;
            std::cout << "logout \t Log out of your account." << std::endl;
            continue;
        } else if(action == "search") {

            std::cout << "### Type * to match anything." << std::endl;

            std::string title, username, extension;
            
            std::cout << "Title: ";
            std::cin >> title;
            std::cout << "Extension: ";
            std::cin >> extension;
            std::cout << "Uploader username: ";
            std::cin >> username;

            request["action"] = "search";
            request["title"] = title;
            request["extension"] = extension;
            request["username"] = username;

        } else if(action == "upload") {
            system("mkdir -p /tmp/part2part/files");

            std::string path;
            bool ok = true;
            do {
                std::cout << "File path: ";
                std::cin >> std::ws;
                std::getline(std::cin, path);
                ok = true;
                if(path.find('.') == std::string::npos) {
                    std::cout << "Please enter a valid file path." << std::endl;
                    ok = false;
                }
                // Check if the file exists
                if(!(access(path.c_str(), F_OK) == 0)) {
                    std::cout << "The file does not exist." << std::endl;
                    continue;
                }
            } while(!ok);
            
            // Open the file
            std::ifstream file(path.c_str(), std::ios::binary);

            // Generate the hash
            std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            std::string hash = SHA256(contents.c_str());

            // Move the file to the uploads directory
            std::string uploadPath = "/tmp/part2part/files/";
            uploadPath += hash + "." + path.substr(path.find_last_of(".") + 1);
            std::ofstream dst(uploadPath.c_str(), std::ios::binary);
            dst << contents;

            std::string title;
            ok = true;
            do {
                std::cout << "Title: ";
                std::cin >> ws;
                std::getline(std::cin, title);
                ok = true;
                if(title.length() < 6) {
                    std::cout << "Please enter at least 6 characters." << std::endl;
                    ok = false;
                }
            } while(!ok);

            request["action"] = "upload";
            request["hash"] = hash;
            request["title"] = title;
            request["extension"] = path.substr(path.find_last_of(".") + 1);
            request["size"] = contents.length();   

        } else if(action == "download") {
            system("mkdir -p downloads");

            unsigned int fileId;
            std::cout << "File ID: ";
            std::cin >> fileId;

            request["action"] = "download";
            request["file_id"] = fileId;

        } else if(action == "register") {
            std::string username;
            std::string password;
            bool ok;
            do {
                std::cout << "Username: ";
                std::cin >> username;
                ok = true;
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
            } while(!ok);
            do {
                std::cout << "Password: ";
                std::cin >> password;
                ok = true;
                if(password.length() < 6) {
                    std::cout << "Please enter at least 6 characters." << std::endl;
                    ok = false;
                }
            } while(!ok);

            request["action"] = "register";
            request["username"] = username;
            request["password"] = password;
            
        } else if(action == "login") {

            bool ok = this->setUpTransferDaemon();
            if(!ok) { 
                continue;
            }

            std::string username, password;

            do {
                std::cout << "Username: ";
                std::cin >> username;
                ok = true;
                if(username.length() < 6) {
                    std::cout << "You username should be at least 6 characters long." << std::endl;
                    ok = false;
                }
            } while(!ok);
            do {
                std::cout << "Password: ";
                std::cin >> password;
                ok = true;
                if(password.length() < 6) {
                    std::cout << "Your password should be at least 6 characters long." << std::endl;
                    ok = false;
                }
            } while(!ok);

            request["action"] = "login";
            request["username"] = username;
            request["password"] = password;
            request["daemon_port"] = this->daemon.getPort(); 
        
        } else if(action == "search") {

            std::string title, extension, username;
            int size;

        } else if(action == "logout") {

            if(this->sessionHash == "0000") {
                std::cout << "### You're already logged out." << std::endl;
            } else {
                this->sessionHash = "0000";
                std::cout << "### You've logged out successfully." << std::endl;
            }

            continue;

        } else {
            std::cout << "### Unknown command. Type 'help' for a list of available commands." << std::endl; 
            continue;
        }

        request["session_hash"] = this->sessionHash;

        if(!this->makeServerRequest(request.dump())) {

            std::cout << "### The server couldn't process your request. Please try again." << std::endl;
        
        } else {

            bool success = (bool) this->response["success"];

            if(!success) {
                
                std::cout << "<<< " << this->response["message"] << std::endl;;

            } else if(action == "login") { // after a successful login

                this->sessionHash = this->response["session_hash"];
                std::cout << "<<< " << this->response["message"] << std::endl;

            } else if(action == "register" || action == "upload") {

                std::cout << "<<< " << this->response["message"] << std::endl;

            } else if(action == "search") {

                if(this->response["success"]) {

                    std::cout << "### The server returned " << this->response["files"].size() << " results." << std::endl; 

                    Table filesTable;
                    filesTable.add_row({"ID", "Title", "Ext.", "Size", "Uploader", "Date"});

                    int k = 0;
                    for(auto f : this->response["files"]) {
                        filesTable.add_row({f["id"], f["title"], f["extension"], f["size"], f["username"], f["date"]});
                        filesTable[++k][0].format()
                            .font_color(Color::yellow)
                            .font_style({FontStyle::bold});
                    }
                    for(size_t i = 0; i < 6; i++) {
                        filesTable[0][i].format()
                            .font_color(Color::blue)
                            .font_align(FontAlign::center)
                            .font_style({FontStyle::bold});
                    }

                    std::cout << filesTable << std::endl;
                    std::cout << "### Type 'download' to download a file." << std::endl;

                }

            } else if(action == "download") {

                std::string ip, port, fileHash, extension;
                ip = (std::string) this->response["ip"];
                port = (std::string) this->response["port"];
                fileHash = (std::string) this->response["file_hash"];
                extension = (std::string) this->response["extension"];

                std::cout << "### Retrieved uploader data. Trying to connect to " << ip << ":" << port << "..." << std::endl;

                if(!this->downloadFile(fileHash, extension, ip, port)) {
                    continue;
                }

            }

        }
    
    }

}

bool Client::makeServerRequest(std::string request) {
    if(!this->connect()) {
        std::cout << "Could not connect to server. Please try again." << std::endl;
        return false;
    }
    request[request.length()] = '\0';
    if(write(this->sd, request.c_str(), request.length()) <= 0) {
        perror("Could not call server");
        return false;
    }
    // Read answer from server
    char answer[2048];
    if(read(this->sd, answer, 2048) < 0) {
        perror("Could not read answer from server");
        return false;
    }

    std::string tmp = answer;

    if(tmp == "null") {
        return false;
    }

    this->response = nlohmann::json::parse(tmp);
    
    if(this->debugging)
        printf("Server answered: %s\n", answer);
        
    return true;
}

void Client::setServer(struct sockaddr_in _server) {
    this->server = _server;
}

bool Client::setUpTransferDaemon() {

    if(this->transferDaemonRunning) {
        return true;
    }

    if(!this->daemon.setup()) {
        std::cout << "Could not set up P2P daemon." << std::endl;
        return false;
    }

    // Create a child process that will handle all the incoming
    // file transfer requests 
    int pid;
    switch(pid = fork()) {
        case -1: { // error
            perror("Error creating child process for the file transfer daemon");
            return false;
        }
        case 0:  { // child process

            this->daemon.watch();
            
        }
        default: { // parent

            return this->transferDaemonRunning = true;

        }
    }
}

bool Client::downloadFile(std::string fileHash, std::string extension, std::string ip, std::string _port) {

    // Connect to the peer to request a file transfer
    int peersd, peersd2; // peer socket descriptor
    struct sockaddr_in peer;
    
    nlohmann::json request;

    request["action"] = "handshake";
    request["file_hash"] = fileHash;
    request["extension"] = extension;

    int port = atoi(_port.c_str());

    // Initiate p2p socket
    if((peersd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("### Encountered error at socket()");
    }

    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = inet_addr(ip.c_str());
    peer.sin_port = htons(port);
    
    // Connect to the peer's daemon
    if(::connect(peersd, (struct sockaddr *) &peer, sizeof (struct sockaddr)) == -1) {
        if(this->debugging)
            perror("Encountered error at connect()");
        std::cout << "### Couldn't connect to peer. They might have exited the app." << std::endl;
        return false;
    }

    std::string tmp = request.dump();
    
    if(write(peersd, tmp.c_str(), tmp.length()) <= 0) {
        if(this->debugging)
            perror("Encountered error at write()");
        std::cout << "### Couldn't send request to peer. Please try again." << std::endl;
        return false;
    }

    std::cout << "### Handshake request sent. Waiting for peer response..." << std::endl;

    char rawResponse[1024];

    if(read(peersd, rawResponse, 1024) < 0) {
        if(this->debugging)
            perror("Encountered error at read()");
        std::cout << "### Couldn't read response from peer. Please try again." << std::endl;
        return false;
    }

    std::string tmp2 = rawResponse;
    int last = 0;
    for(int i = 0; i < tmp2.length(); i++) {
        if(tmp2[i] == '}') {
            last = i;
        }
    }
    tmp2 = tmp2.substr(0, last+1);

    nlohmann::json response;
    response = nlohmann::json::parse(tmp2.c_str());

    close(peersd);

    if(response["success"]) {
        std::cout << "### Peer answered. Sending file transfer request..." << std::endl;

        // Initiate the second p2p socket
        if((peersd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("### Encountered error at socket()");
        }

        // Connect to the peer's daemon
        if(::connect(peersd2, (struct sockaddr *) &peer, sizeof (struct sockaddr)) == -1) {
            if(this->debugging)
                perror("Encountered error at connect()");
            std::cout << "### Couldn't connect to peer to download file. They might have exited the app." << std::endl;
            return false;
        }

        if(this->debugging) {
            std::cout << "Peer answer: " << tmp2 << std::endl;
        }
        
        request["action"] = "download";

        long fileSize = response["size"];
        std::cout << fileSize << "bytes" << std::endl;
        int chunks = fileSize / BUFFER_SIZE; 
        if(fileSize % BUFFER_SIZE > 0) {
            chunks += 1;
        }

        tmp = request.dump();
    
        if(write(peersd2, tmp.c_str(), tmp.length()) < 0) {
            if(this->debugging)
                perror("Encountered error at write()");
            std::cout << "### Couldn't send request to peer. Please try again." << std::endl;
            close(peersd2);
            return false;
        }

        std::string path = "downloads/";
        // Get current timestamp
        std::time_t t = std::time(0);
        path += std::to_string(t);
        path += "_" + fileHash +  "." + extension;
        
        std::ofstream file(path.c_str(), std::ios::binary);
        
        int k = 0;
        while(fileSize > 0) {

            std::vector<char> buffer;
            buffer.reserve(BUFFER_SIZE);

            std::cout << "Downloading chunk " << ++k << "/" << chunks;

            int readLength = read(peersd2, &buffer[0], BUFFER_SIZE);

            if(readLength < 0) {
                if(this->debugging)
                    perror("Encountered error at read()");
                std::cout << "### Couldn't read chunk from peer. Please try again." << std::endl;
                return false;
            }
            file.write((char *)&buffer[0], readLength);

            fileSize -= readLength;

            std::cout << " (" << readLength << " bytes)" << std::endl;

        }

        std::cout << "### Saved file as " << path << std::endl;

        return true;
        
    } else {
        std::cout << "<<< " << response["message"] << std::endl;
        close(peersd);
        return false;
    }

}