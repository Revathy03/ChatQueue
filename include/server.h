#ifndef SERVER_H
#define SERVER_H

#include "dbconnection.h"
#include <string>

class Server
{
private:
    DBConnection db;

public:
    Server(const std::string &host,
           int port,
           const std::string &user,
           const std::string &password,
           const std::string &db_name);

    void start();
};

#endif
