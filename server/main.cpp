#define DB_PATH "./database/data.sqlite"
#include "class/Server.cpp"

int main() {
    Server s;
    s.setDebugging(true);
    s.watch();
    return 0;
}