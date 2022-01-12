#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h> 
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include "SHA256.cpp"
#include "json.hpp"

struct User {
    int id;
    std::string username;
    std::string password;
    std::time_t createdAt;
};

struct Download {
    int id;
    int downloaderId;
    int fileId;
};

struct Session {
    int id;
    int userId;
    std::string sessionHash;
    std::string ip;
    std::string port;
};

struct File {
    int id;
    std::string hash;
    std::string title;
    int size;
    std::string extension;
    int uploaderId;
    std::time_t createdAt;
    std::string username;
    std::string createdAtString;
    Session lastLogin;
};

class Database 
{
private:
    sqlite3 *db; // pointer to a sqlite database instance
    char* sql; // database query
    bool connected; // whether this->db points to an active SQLite session
    bool debugging; // whether the debugging tool is activated
    bool initialize(); // initialize the database structure
    std::string replaceAll(std::string, const std::string&, const std::string&); // replace all occurences of a substring
public:
    Database(); // default constructor
    char* msg; // SQL query 
    std::string messageForClient; // a message that should be forwarded to the peer by the server
    bool connect(const char*); // connect to the database
    void setDebugging(bool); // toggle the debugging tool
    User getUser(int); // get an user
    User getUser(std::string); // get an user by username
    bool createUser(User); // register a new user
    File getFile(int); // get a file given an id
    File getFile(int, std::string, std::string); // get a file given an user_id, hash and extension
    bool createFile(File); // register a new file
    Download getDownload(int); // get a download
    bool createDownload(Download); // register a new download
    Session getSession(std::string); // get a session by hash
    bool createSession(Session); // register a new session
    std::vector<File> searchFiles(std::string, std::string, std::string); // search files
};

#endif
