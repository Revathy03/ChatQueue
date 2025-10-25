#include "dbconnection.h"
#include <iostream>

using namespace std;

DBConnection::DBConnection(const std::string &host,
                           int port,
                           const std::string &user,
                           const std::string &password,
                           const std::string &db_name)
    : session(mysqlx::Session(host, port, user, password)),
      db(session.getSchema(db_name))
{
    cout << "Connected to DB successfully." << endl;
}

DBConnection::~DBConnection()
{
    session.close();
}

void DBConnection::insertMessage(int sender_id, int receiver_id, const std::string &msg)
{
    try
    {
        mysqlx::Table messages = db.getTable("messages");
        messages.insert("sender_id", "receiver_id", "msg")
            .values(sender_id, receiver_id, msg)
            .execute();
        cout << "Message inserted successfully.\n";
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Insert Error: " << err.what() << endl;
    }
}

mysqlx::RowResult DBConnection::getAllMessages()
{
    mysqlx::Table messages = db.getTable("messages");
    return messages.select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
        .execute();
}
