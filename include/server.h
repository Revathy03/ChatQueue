#ifndef SERVER_H
#define SERVER_H

#include "dbconnection.h"
#include "cache.h"
#include <string>

class Server
{
private:
    DBConnection db;
    Cache c;
    unordered_map<int, int> unreadCnt;
    mutable std::mutex unreadMtx;

public:
    Server(const std::string &host,
           int port,
           const std::string &user,
           const std::string &password,
           const std::string &db_name);

    void start();
    
};

#endif
