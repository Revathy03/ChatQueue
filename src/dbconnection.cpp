#include "dbconnection.h"
#include <iostream>

using namespace std;

// --- Acquire a free session from pool ---
DBConnection::PooledSession *DBConnection::acquireSession()
{
    std::unique_lock<std::mutex> lock(pool_mtx);
    pool_cv.wait(lock, [this]
                 {
        for (auto &s : pool) if (!s->inUse) return true;
        return false; });

    for (auto &s : pool)
    {
        if (!s->inUse)
        {
            s->inUse = true;
            return s.get();
        }
    }
    return nullptr; // should never happen
}

// --- Release session back to pool ---
void DBConnection::releaseSession(PooledSession *sess)
{
    {
        std::lock_guard<std::mutex> lock(pool_mtx);
        sess->inUse = false;
    }
    pool_cv.notify_one();
}

// --- Get schema ---
mysqlx::Schema DBConnection::getSchema(mysqlx::Session &session)
{
    return session.getSchema(db_name);
}

// --- Constructor ---
DBConnection::DBConnection(const std::string &host,
                           int port,
                           const std::string &user,
                           const std::string &password,
                           const std::string &db_name)
    : db_name(db_name)
{
    // Create pool
    for (int i = 0; i < pool_size; i++)
    {
        pool.push_back(make_unique<PooledSession>(host, port, user, password));
    }

    // Ensure DB & tables exist using first session
    auto sess = acquireSession();
    try
    {
        sess->session->sql("CREATE DATABASE IF NOT EXISTS " + db_name).execute();
        sess->session->sql("USE " + db_name).execute();

        sess->session->sql(
                         "CREATE TABLE IF NOT EXISTS messages ("
                         "id INT AUTO_INCREMENT PRIMARY KEY,"
                         "sender_id INT NOT NULL,"
                         "receiver_id INT NOT NULL,"
                         "msg TEXT NOT NULL,"
                         "is_read BOOLEAN DEFAULT FALSE,"
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)")
            .execute();

        sess->session->sql(
                         "CREATE TABLE IF NOT EXISTS clients ("
                         "id INT AUTO_INCREMENT PRIMARY KEY,"
                         "is_active BOOLEAN DEFAULT FALSE)")
            .execute();

        cout << "Database and table check/creation completed" << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "DB/Table Creation Error: " << err.what() << endl;
    }
    releaseSession(sess);
}

// --- Destructor ---
DBConnection::~DBConnection()
{
    for (auto &s : pool)
    {
        if (s->session)
            s->session->close();
    }
}

// --- Methods ---
int DBConnection::registerClient()
{
    auto sess = acquireSession();
    int client_id = -1;

    try
    {
        auto result = sess->session->sql(
                                       "SELECT id FROM clients WHERE is_active = FALSE ORDER BY id LIMIT 1")
                          .execute();
        auto row = result.fetchOne();

        if (row)
        {
            client_id = row[0].get<int>();
            sess->session->sql("UPDATE clients SET is_active = TRUE WHERE id = ?")
                .bind(client_id)
                .execute();
            cout << "Reactivated Client ID: " << client_id << endl;
        }
        else
        {
            sess->session->sql("INSERT INTO clients (is_active) VALUES (TRUE)").execute();
            auto insert_row = sess->session->sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
            client_id = insert_row[0].get<int>();
            cout << "New Client Registered: " << client_id << endl;
        }
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "registerClient Error: " << err.what() << endl;
    }

    releaseSession(sess);
    return client_id;
}

void DBConnection::deactivateClient(int client_id)
{
    auto sess = acquireSession();
    try
    {
        auto stmt = sess->session->sql("UPDATE clients SET is_active = FALSE WHERE id = ?")
                        .bind(client_id)
                        .execute();

        if (stmt.getAffectedItemsCount() > 0)
            cout << "Client marked inactive: " << client_id << endl;
        else
            cout << "Client ID not found: " << client_id << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error during deactivation: " << e.what() << endl;
    }
    releaseSession(sess);
}

void DBConnection::insertMessage(int sender_id, int receiver_id,
                                 const std::string &msg, const std::string &created_at)
{
    auto sess = acquireSession();
    try
    {
        auto db = getSchema(*sess->session);
        db.getTable("messages").insert("sender_id", "receiver_id", "msg", "created_at").values(sender_id, receiver_id, msg, created_at).execute();
     //   cout << "Message inserted @ " << created_at << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Insert Error: " << err.what() << endl;
    }
    releaseSession(sess);
}

mysqlx::RowResult DBConnection::getMessages(int receiver_id)
{
    auto sess = acquireSession();
    mysqlx::RowResult result;
    try
    {
        auto db = getSchema(*sess->session);
        result = db.getTable("messages").select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at").where("receiver_id = :rid AND is_read = FALSE").bind("rid", receiver_id).execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "getMessages Error: " << err.what() << endl;
    }
    releaseSession(sess);
    return result;
}

void DBConnection::markMessagesAsRead(int receiver_id)
{
    auto sess = acquireSession();
    try
    {
        auto db = getSchema(*sess->session);
        db.getTable("messages").update().set("is_read", true).where("receiver_id = :rid AND is_read = FALSE").bind("rid", receiver_id).execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Mark as read failed: " << err.what() << endl;
    }
    releaseSession(sess);
}

mysqlx::RowResult DBConnection::getHistory(int receiver_id)
{
    auto sess = acquireSession();
    mysqlx::RowResult result;
    try
    {
        auto db = getSchema(*sess->session);
        result = db.getTable("messages").select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at").where("receiver_id = :rid").bind("rid", receiver_id).execute();

        db.getTable("messages").update().set("is_read", true).where("receiver_id = :rid AND is_read = FALSE").bind("rid", receiver_id).execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "getHistory Error: " << err.what() << endl;
    }
    releaseSession(sess);
    return result;
}

void DBConnection::deleteMessagesByReceiver(int receiver_id)
{
    auto sess = acquireSession();
    try
    {
        auto db = getSchema(*sess->session);
        db.getTable("messages").remove().where("receiver_id = :rid").bind("rid", receiver_id).execute();
        cout << "Messages deleted for receiver_id: " << receiver_id << endl;
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "Delete Error: " << err.what() << endl;
    }
    releaseSession(sess);
}

mysqlx::RowResult DBConnection::getUnreadMessagesBefore(int receiver_id, const std::string &timestamp)
{
    auto sess = acquireSession();
    mysqlx::RowResult result;
    try
    {
        auto db = getSchema(*sess->session);
        result = db.getTable("messages").select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at").where("receiver_id = :rid AND is_read = FALSE AND created_at < :ts").orderBy("created_at DESC").bind("rid", receiver_id).bind("ts", timestamp).execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "getUnreadMessagesBefore Error: " << err.what() << endl;
    }
    releaseSession(sess);
    return result;
}
mysqlx::RowResult DBConnection::getRecentMessages(int receiver_id, int limit = 50)
{
    auto sess = acquireSession();
    mysqlx::RowResult result;
    try
    {
        auto db = getSchema(*sess->session);
        result = db.getTable("messages")
                     .select("id", "sender_id", "receiver_id", "msg", "CAST(created_at AS CHAR) AS created_at")
                     .where("receiver_id = :rid")
                     .orderBy("created_at DESC")
                     .limit(limit)
                     .bind("rid", receiver_id)
                     .execute();
    }
    catch (const mysqlx::Error &err)
    {
        cerr << "getRecentMessages Error: " << err.what() << endl;
    }
    releaseSession(sess);
    return result;
}
