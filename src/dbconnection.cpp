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

    try
    {
        session.sql("CREATE DATABASE IF NOT EXISTS " + db_name).execute();
        session.sql("USE " + db_name).execute();

        session.sql("CREATE TABLE IF NOT EXISTS messages ("
                    "id INT AUTO_INCREMENT PRIMARY KEY,"
                    "sender_id INT NOT NULL,"
                    "receiver_id INT NOT NULL,"
                    "msg TEXT NOT NULL,"
                    "is_read BOOLEAN DEFAULT FALSE,"
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                    ")")
            .execute();

        session.sql("CREATE TABLE IF NOT EXISTS clients ("
                    "id INT AUTO_INCREMENT PRIMARY KEY,"
                    "is_active BOOLEAN DEFAULT FALSE"
                    ")")
            .execute();

        cout << "Table check/creation completed" << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Table Creation Error: " << err.what() << endl;
    }
}

/*
mysqlx::Session(host, port, user, password) - Opens a session using MySQL X DevAPI.
session.getSchema(db_name) - Gets a handle to the database/schema you want to work with.
*/

DBConnection::~DBConnection()
{
    session.close();
}

int DBConnection::registerClient()
{
    lock_guard<mutex> lock(mtx);
    // Step 1: Try to reuse an inactive client ID
    auto result = session.sql(
                             "SELECT id FROM clients WHERE is_active = FALSE ORDER BY id LIMIT 1")
                      .execute();

    auto row = result.fetchOne();
    if (row)
    {
        int reused_id = row[0].get<int>();

        // Mark it active again
        session.sql("UPDATE clients SET is_active = TRUE WHERE id = ?")
            .bind(reused_id)
            .execute();

        cout << "Reactivated Client ID: " << reused_id << endl;
        return reused_id;
    }

    // Step 2: No inactive found → Create a new row
    session.sql("INSERT INTO clients (is_active) VALUES (TRUE)").execute();

    auto insert_result = session.sql("SELECT LAST_INSERT_ID()").execute();
    auto insert_row = insert_result.fetchOne();
    int new_id = insert_row[0].get<int>();

    cout << "New Client Registered: " << new_id << endl;
    return new_id;
}

void DBConnection::deactivateClient(int client_id)
{
    lock_guard<mutex> lock(mtx);
    try
    {
        auto stmt = session.sql(
                               "UPDATE clients SET is_active = FALSE WHERE id = ?")
                        .bind(client_id)
                        .execute();

        if (stmt.getAffectedItemsCount() > 0)
        {
            cout << "Client marked inactive: " << client_id << endl;
        }
        else
        {
            cout << "Client ID not found: " << client_id << endl;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error during deactivation: " << e.what() << endl;
    }
}

void DBConnection::insertMessage(int sender_id, int receiver_id,
                                 const std::string &msg, const std::string &created_at)
{
    lock_guard<mutex> lock(mtx);
    try
    {
        mysqlx::Table messages = db.getTable("messages");
        messages.insert("sender_id", "receiver_id", "msg", "created_at")
            .values(sender_id, receiver_id, msg, created_at)
            .execute();
        cout << "Message inserted @ " << created_at << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Insert Error: " << err.what() << endl;
    }
}

mysqlx::RowResult DBConnection::getMessages(int receiver_id)
{
    lock_guard<mutex> lock(mtx);
    mysqlx::Table messages = db.getTable("messages");

    // First: fetch unread messages
    mysqlx::RowResult result = messages.select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
                                .where("receiver_id = :rid AND is_read = FALSE")
                                .bind("rid", receiver_id)
                                .execute();

    return result;
}

void DBConnection::markMessagesAsRead(int receiver_id)
{
    lock_guard<mutex> lock(mtx);
    try
    {
        mysqlx::Table messages = db.getTable("messages");
        messages.update()
            .set("is_read", true)
            .where("receiver_id = :rid AND is_read = FALSE")
            .bind("rid", receiver_id)
            .execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Mark as read failed: " << err.what() << endl;
    }
}

mysqlx::RowResult DBConnection::getHistory(int receiver_id)
{
    lock_guard<mutex> lock(mtx);
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
    lock_guard<mutex> lock(mtx);
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

mysqlx::RowResult DBConnection::getUnreadMessagesBefore(int receiver_id, const std::string &timestamp)
{
    lock_guard<mutex> lock(mtx);
    mysqlx::Table messages = db.getTable("messages");

    try
    {
        mysqlx::RowResult result = messages
                                       .select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
                                       .where("receiver_id = :rid AND is_read = FALSE AND created_at < :ts")
                                       .orderBy("created_at DESC")
                                       .bind("rid", receiver_id)
                                       .bind("ts", timestamp)
                                       .execute();

        return result;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Error fetching unread messages before timestamp: " << err.what() << endl;
        throw;
    }
}
