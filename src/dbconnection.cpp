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

/*
mysqlx::Session(host, port, user, password) - Opens a session using MySQL X DevAPI.
session.getSchema(db_name) - Gets a handle to the database/schema you want to work with.
*/

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

mysqlx::RowResult DBConnection::getMessages(int receiver_id)
{
    mysqlx::Table messages = db.getTable("messages");

    // First: fetch unread messages
    mysqlx::RowResult result = messages.select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
                                .where("receiver_id = :rid AND is_read = FALSE")
                                .bind("rid", receiver_id)
                                .execute();

    // Second: mark them as read
    messages.update()
        .set("is_read", true)
        .where("receiver_id = :rid AND is_read = FALSE")
        .bind("rid", receiver_id)
        .execute();

    return result;
}

mysqlx::RowResult DBConnection::getHistory(int receiver_id)
{
    mysqlx::Table messages = db.getTable("messages");

    // First: fetch unread messages
    mysqlx::RowResult result = messages.select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
                                   .where("receiver_id = :rid")
                                   .bind("rid", receiver_id)
                                   .execute();

    // Second: mark them as read
    messages.update()
        .set("is_read", true)
        .where("receiver_id = :rid AND is_read = FALSE")
        .bind("rid", receiver_id)
        .execute();

    return result;
}

void DBConnection::deleteMessagesByReceiver(int receiver_id)
{
    try
    {
        mysqlx::Table messages = db.getTable("messages");
        messages.remove()
            .where("receiver_id = :rid")
            .bind("rid", receiver_id)
            .execute();

        cout << "Messages deleted for receiver_id: " << receiver_id << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Delete Error: " << err.what() << endl;
    }
}
