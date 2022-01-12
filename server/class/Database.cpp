#include "Database.h"

static int numOfRows(void *count, int argc, char **argv, char **azColName) {
    int *c = (int*) count;
    *c = atoi(argv[0]);
    return 0;
}

static int selectUserCallback(void *user, int argc, char **argv, char **azColName) {
    User *u = (User*) user;
    u->id = atoi(argv[0]);
    u->username = argv[1];
    u->password = argv[2];
    return 0;
}

static int selectFileCallback(void *file, int argc, char **argv, char **azColName) {
    File *f = (File*) file;
    f->id = atoi(argv[0]);
    f->hash = argv[1];
    f->uploaderId = atoi(argv[2]);
    f->title = argv[3];
    f->size = atoi(argv[4]);
    f->extension = argv[5];
    return 0;
}

static int selectDownloadFileCallback(void *file, int argc, char **argv, char **azColName) {
    File *f = (File*) file;
    f->id = atoi(argv[0]);
    f->hash = argv[4];
    f->extension = argv[5];
    f->lastLogin.id = atoi(argv[1]);
    f->lastLogin.ip = argv[2];
    f->lastLogin.port = argv[3];
    return 0;
}

static int searchFilesCallback(void *ptr, int argc, char **argv, char **azColName) {
    std::vector<File> *files = reinterpret_cast<std::vector<File>*>(ptr);
    File f;
    f.id = atoi(argv[0]);
    f.title = argv[1];
    f.size = atoi(argv[2]);
    f.extension = argv[3];
    f.createdAtString = argv[4];
    f.username = argv[5];
    files->push_back(f);    
    return 0;
}

static int selectSessionCallback(void *session, int argc, char **argv, char **azColName) {
    Session *s = (Session*)session;
    s->id = atoi(argv[0]);
    s->userId = atoi(argv[1]);
    s->sessionHash = argv[2];
    s->ip = argv[3];
    s->port = argv[4];
    return 0;
}

Database::Database() {
    this->msg = 0;
    this->connected = false;
}

bool Database::connect(const char* path) {
    int rc = sqlite3_open(path, &this->db); // response code
    if(rc) { // if database couldn't be opened
        std::cout << "[" << getpid() << "] Cannot open " << path << ": " << sqlite3_errmsg(this->db);
        return false;
    } else { // if database opened successfully
        this->connected = true;
        if(this->debugging)
            std::cout << "[" << getpid() << "] Database connection established successfully." << std::endl;
        if(!this->initialize()) {
            return false;
        }
        return true;
    }
}

void Database::setDebugging(bool d) {
    this->debugging = d;
}

bool Database::initialize() {
    const char* sql = 
      "CREATE TABLE IF NOT EXISTS USERS("
        "ID             INTEGER PRIMARY KEY NOT NULL,"
        "USERNAME       TEXT                NOT NULL,"
        "PASSWORD       TEXT                NOT NULL,"
        "CREATED_AT     DATETIME            NOT NULL    DEFAULT CURRENT_TIMESTAMP);"
      "CREATE TABLE IF NOT EXISTS FILES("
        "ID             INTEGER PRIMARY KEY NOT NULL,"
        "HASH           TEXT                NOT NULL,"
        "UPLOADER_ID    INTEGER             NOT NULL,"
        "TITLE          TEXT                NOT NULL,"
        "SIZE           INT                 NOT NULL,"
        "EXTENSION      TEXT                NOT NULL,"
        "CREATED_AT     DATETIME            NOT NULL    DEFAULT CURRENT_TIMESTAMP);"
      "CREATE TABLE IF NOT EXISTS DOWNLOADS("
        "ID             INTEGER PRIMARY KEY NOT NULL,"
        "DOWNLOADER_ID  INTEGER             NOT NULL,"
        "FILE_ID        INTEGER             NOT NULL,"
        "CREATED_AT     DATETIME            NOT NULL    DEFAULT CURRENT_TIMESTAMP);"
      "CREATE TABLE IF NOT EXISTS SESSIONS("
        "ID             INTEGER PRIMARY KEY NOT NULL,"
        "USER_ID        INTEGER             NOT NULL,"
        "HASH           TEXT                NOT NULL,"
        "IP             TEXT                NOT NULL,"
        "PORT           TEXT                NOT NULL,"
        "CREATED_AT     DATETIME            NOT NULL    DEFAULT CURRENT_TIMESTAMP);";

    int rc = sqlite3_exec(this->db, sql, NULL, 0, &this->msg);
   
    if(rc != SQLITE_OK){
        fprintf(stderr, "[%d] SQL error: %s\n", getpid(), this->msg);
        sqlite3_free(this->msg);
        return false;
    } else {
        if(this->debugging)
            std::cout << "[" << getpid() << "] Database tables are ready." << std::endl;
        return true;
    }
}

User Database::getUser(std::string username) {

    User u;
    u.id = -1;

    std::string lowercaseUsername = username;

    std::for_each(lowercaseUsername.begin(), lowercaseUsername.end(), [](char & c) {
        c = ::tolower(c);
    });

    std::string tmp = "SELECT * FROM USERS WHERE LOWER(USERNAME) = ";
    tmp += "'" + replaceAll(lowercaseUsername, "'", "''");
    tmp += "' LIMIT 1;";

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    const char* sql = tmp.c_str();

    int rc = sqlite3_exec(this->db, tmp.c_str(), selectUserCallback, &u, &this->msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while getting user: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "We couldn't log you in. Please try again later.";
        sqlite3_free(this->msg);
    }

    return u;

}

bool Database::createUser(User u) { 
    std::string tmp;
    int rc;
    int count = 0;

    std::string lowercaseUsername = u.username;

    std::for_each(lowercaseUsername.begin(), lowercaseUsername.end(), [](char & c) {
        c = ::tolower(c);
    });
 
    // check whether the username has already been taken
    tmp = "SELECT COUNT(*) FROM USERS WHERE LOWER(USERNAME) = ";
    tmp += "'" + replaceAll(lowercaseUsername, "'", "''");
    tmp += "' LIMIT 1;";

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    rc = sqlite3_exec(this->db, tmp.c_str(), numOfRows, &count, &this->msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while checking username usage: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "We couldn't register you in our database. Please try again later.";
        sqlite3_free(this->msg);
        return false;
    }
    if(count > 0) {
        this->messageForClient = "This username has already been taken. Please try another one.";
        return false;
    }

    tmp = "INSERT INTO "
        "USERS("
            "USERNAME, "
            "PASSWORD" 
        ")"
        " VALUES (";
    tmp += "'" + replaceAll(u.username, "'", "''") + "', ";
    tmp += "'" + u.password + "');";
    const char* sql = tmp.c_str();

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, NULL, 0, &this->msg);
   
    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while creating new user: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "We couldn't register you in our database. Please try again later.";
        sqlite3_free(this->msg);
        return false;
    }
    return true;

}

Session Database::getSession(std::string hash) {

    Session s;
    s.userId = -1;

    std::string tmp;
    int rc;

    tmp = "SELECT * FROM SESSIONS WHERE hash = '";
    tmp += hash;
    tmp += "' LIMIT 1;";

    const char* sql = tmp.c_str();

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    //selectSessionCallback
    rc = sqlite3_exec(this->db, sql, selectSessionCallback, &s, &this->msg);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while selecting session: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "Please log in again.";
        sqlite3_free(this->msg);
    }
    
    return s;

}

File Database::getFile(int uploaderId, std::string hash, std::string extension) {

    File f;
    f.id = -1;

    std::string tmp;
    int rc;

    char* sql = new char[512];
    sprintf(sql, "SELECT * FROM FILES "
                 "WHERE uploader_id = '%d' AND hash = '%s' AND extension = '%s' "
                 "LIMIT 1",
            uploaderId, hash.c_str(), extension.c_str());

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << sql << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, selectFileCallback, &f, &this->msg);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while selecting file: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "An unexpected error occurred. Please try again later.";
        sqlite3_free(this->msg);
    }
    
    return f;

}

File Database::getFile(int fileId) {

    File f;
    f.id = -1;

    std::string tmp;
    int rc;

    char* sql = new char[512];
    sprintf(sql, "SELECT f.id, s.id, s.ip, s.port, f.hash, f.extension "
                 "FROM files f "
                 "JOIN sessions s ON s.user_id = f.uploader_id "
                 "WHERE f.id = %d "
                 "ORDER BY s.id DESC "
                 "LIMIT 1;",
            fileId);

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << sql << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, selectDownloadFileCallback, &f, &this->msg);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while selecting file: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "An unexpected error occurred. Please try again later.";
        sqlite3_free(this->msg);
    }
    
    return f;

}

bool Database::createSession(Session s) {

    std::string tmp;
    int rc;

    tmp = "INSERT INTO "
        "SESSIONS("
            "USER_ID, "
            "HASH, "
            "IP, "
            "PORT"
        ")"
        " VALUES (";
    tmp += "'" + std::to_string(s.userId) + "', ";
    tmp += "'" + s.sessionHash + "', ";
    tmp += "'" + s.ip + "', ";
    tmp += "'" + s.port + "');";
    const char* sql = tmp.c_str();

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, NULL, 0, &this->msg);
   
    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while creating session: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "We couldn't log you in. Please try again later.";
        sqlite3_free(this->msg);
        return false;
    }

    return true;

}

bool Database::createFile(File f) {

    int rc;

    File tmpf = this->getFile(f.uploaderId, f.hash, f.extension);
    if(tmpf.id > -1) {
        this->messageForClient = "You've already uploaded this file.";
        return false;
    }

    std::string tmp = "INSERT INTO "
        "FILES("
            "HASH, "
            "UPLOADER_ID, "
            "TITLE, "
            "SIZE, "
            "EXTENSION"
        ")"
        " VALUES (";
    tmp += "'" + f.hash + "', ";
    tmp += std::to_string(f.uploaderId) + ", ";
    tmp += "'" + f.title + "', ";
    tmp += std::to_string(f.size) + ", ";
    tmp += "'" + f.extension + "');";
    const char* sql = tmp.c_str();

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << tmp << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, NULL, 0, &this->msg);
   
    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while creating file: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "We couldn't register your file. Please try again later.";
        sqlite3_free(this->msg);
        return false;
    }

    return true;

}

std::string Database::replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

std::vector<File> Database::searchFiles(std::string title, std::string extension, std::string username) {

    std::vector<File> files;
    int rc;

    char* sql = new char[1024];
    sprintf(sql, "SELECT f.id, f.title, f.size, f.extension, f.created_at, u.username "
            "FROM files f "
            "JOIN users u ON u.id = f.uploader_id "
            "WHERE LOWER(title) LIKE '%%%s%%' "
              "AND LOWER(extension) LIKE '%%%s%%' "
              "AND LOWER(username) LIKE '%%%s%%' "
            "LIMIT 10;", 
        title.c_str(), 
        extension.c_str(), 
        username.c_str()
    );

    if(this->debugging) {
        std::cout << "[" << getpid() << "] Executing " << sql << std::endl;
    }

    rc = sqlite3_exec(this->db, sql, searchFilesCallback, &files, &this->msg);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "[%d] SQL error %d while searching files: %s\n", getpid(), rc, this->msg);
        this->messageForClient = "An unexpected error occurred. Please try again later.";
        sqlite3_free(this->msg);
    }
    
    return files;

}