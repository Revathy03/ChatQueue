#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <mysqlx/xdevapi.h>
#include <string>

class DBConnection
{
private:
    mysqlx::Session session; // Represents the connection session with the MySQL server.
    mysqlx::Schema db;       // Represents a specific database/schema selected within the session.

public:
    DBConnection(const std::string &host,
                 int port,
                 const std::string &user,
                 const std::string &password,
                 const std::string &db_name);

    ~DBConnection();

    void insertMessage(int sender_id, int receiver_id, const std::string &msg);
    mysqlx::RowResult getMessages(int receiver_id);
    mysqlx::RowResult getHistory(int receiver_id);
    void deleteMessagesByReceiver(int receiver_id);
};

#endif
