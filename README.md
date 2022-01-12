# Part2Part - CTP/IP file transfer in C++

### Usage

First, make sure you have SQLite installed on your machine.

For compiling and starting the Server and the Client, run:

```cd server
g++ -std=c++11 main.cpp -o part2part_server -lsqlite3 && clear && ./part2part_server
```

```cd client
g++ -std=c++11 main.cpp -o part2part_client -lsqlite3 && clear && ./part2part_client 127.0.0.1 33333
```