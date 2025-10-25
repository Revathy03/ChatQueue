#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <mysqlx/xdevapi.h>
#include <string>

class DBConnection
{
private:
    mysqlx::Session session;
    mysqlx::Schema db;

public:
    DBConnection(const std::string &host,
                 int port,
                 const std::string &user,
                 const std::string &password,
                 const std::string &db_name);

    ~DBConnection();

    void insertMessage(int sender_id, int receiver_id, const std::string &msg);
    mysqlx::RowResult getAllMessages();
};

#endif
