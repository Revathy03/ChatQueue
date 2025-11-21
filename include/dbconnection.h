#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <mysqlx/xdevapi.h>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <condition_variable>

class DBConnection
{
private:
    struct PooledSession
    {
        std::unique_ptr<mysqlx::Session> session;
        bool inUse = false;

        PooledSession(const std::string &host, int port,
                      const std::string &user, const std::string &password)
            : session(std::make_unique<mysqlx::Session>(host, port, user, password)) {}
    };

    std::vector<std::unique_ptr<PooledSession>> pool;
    mutable std::mutex pool_mtx;
    std::condition_variable pool_cv;
    int pool_size = 100;

    std::string db_name;

    // Helper to acquire/release pooled sessions
    PooledSession *acquireSession();
    void releaseSession(PooledSession *sess);

    // Helper to get mysqlx::Schema
    mysqlx::Schema getSchema(mysqlx::Session &session);

public:
    DBConnection(const std::string &host,
                 int port,
                 const std::string &user,
                 const std::string &password,
                 const std::string &db_name);

    ~DBConnection();

    int registerClient();
    void deactivateClient(int client_id);
    void insertMessage(int sender_id, int receiver_id, const std::string &msg, const std::string &created_at);
    mysqlx::RowResult getMessages(int receiver_id);
    void markMessagesAsRead(int receiver_id);
    mysqlx::RowResult getHistory(int receiver_id);
    void deleteMessagesByReceiver(int receiver_id);
    mysqlx::RowResult getUnreadMessagesBefore(int receiver_id, const std::string &timestamp);
    mysqlx::RowResult getRecentMessages(int receiver_id, int limit);
};

#endif
