#include "server.h"
#include "httplib.h"
#include "json.hpp"
#include <iostream>

using namespace std;
using json = nlohmann::json;

Server::Server(const string &host,
               int port,
               const string &user,
               const string &password,
               const string &db_name)
    : db(host, port, user, password, db_name),c(5000,5000) {} // As the server starts it connects to db, calling the constructor of DBconnection
//add ,c(2,3) to change limit
void Server::start()
{
    httplib::Server svr; // creating a server object

    cout << "REST Server running on http://localhost:8080" << endl;

    // Insert message
    //[&]	Captures variables by reference from outer scope (so you can use db inside)
    // const httplib::Request &req	Contains everything sent by the client: body, headers, URL parameters
    // httplib::Response &res	You fill this to send data back: status code, response JSON, etc.

    /*
    req.body            // The JSON or text sent by client
    req.method          // "PUT"
    req.path            // "/message"
    req.headers         // HTTP headers

    */
    svr.Post("/register", [&](const httplib::Request &req, httplib::Response &res)
             {
                 int client_id = db.registerClient();
                 string json = "{\"client_id\": " + to_string(client_id) + "}";
                 res.status = 200;
                 res.set_content(json, "application/json");
                 {std::lock_guard<std::mutex> lock(unreadMtx);
                 unreadCnt[client_id]=0; }});

    svr.Post("/message", [&](const httplib::Request &req, httplib::Response &res)
             {
        try {
            json body = json::parse(req.body);

            /*
            json body = json::parse(req.body);
            body["sender_id"]
            body["receiver_id"]
            body["msg"]
            */

            Message msg;
            msg.senderId = body["sender_id"].get<int>();
            msg.text = body["msg"];

            // Assign timestamp here
            auto now = chrono::system_clock::now();
            time_t t = chrono::system_clock::to_time_t(now);
            tm local_tm = *localtime(&t);
            stringstream ss;
            ss << put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
            msg.timestamp = ss.str();

            int receiver_id = body["receiver_id"].get<int>();

            // Insert into cache immediately
            c.insertMessage(receiver_id, msg);
            c.printCache();

            // Send response first (non-blocking)
            res.status = 200;
            res.set_content(R"({"status":"Message stored"})", "application/json");
            {std::lock_guard<std::mutex> lock(unreadMtx);
            unreadCnt[receiver_id]++;}
            // Then update DB asynchronously (fire-and-forget)
            std::thread([&, msg, receiver_id]()
                        { db.insertMessage(msg.senderId, receiver_id, msg.text, msg.timestamp); })
                .detach();
            //db.insertMessage(msg.senderId, receiver_id, msg.text, msg.timestamp);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid request"})", "application/json");
        } 
    });



    // Get messages for client
    /*
    \d+ - Capture a number from the URL
    matches[0]	The whole matched URL (/message/42)
    matches[1]	The first captured group → 42
    */
    svr.Get(R"(/message/(\d+))", [&](const httplib::Request &req, httplib::Response &res)
            {
    int client_id = stoi(req.matches[1]);
    json messages = c.readUnreadMessages(client_id);
    c.printCache();

    int cachedCount = messages.size();
    int totalUnread = 0;
    {std::lock_guard<std::mutex> lock(unreadMtx);
    totalUnread = unreadCnt[client_id];}

    cout << "Cache had " << cachedCount << " / " << totalUnread << " unread messages.\n";

    if (cachedCount < totalUnread) {
        string oldestTs = cachedCount > 0
                              ? (messages.back().contains("timestamp")
                                     ? messages.back()["timestamp"].get<string>()
                                     : messages.back()["created_at"].get<string>())
                              : "9999-12-31 23:59:59";

        auto dbResult = db.getUnreadMessagesBefore(client_id, oldestTs);

        for (auto row : dbResult) {
            messages.push_back({
                {"id", int(row[0])},
                {"sender_id", int(row[1])},
                {"receiver_id", int(row[2])},
                {"msg", (string)row[3]},
                {"created_at", row[4].get<string>()}  // unified key
            });
        }
    }

    if (messages.is_null()) messages = json::array();

    res.status = 200;
    res.set_content(messages.dump(), "application/json");
    {std::lock_guard<std::mutex> lock(unreadMtx);
    unreadCnt[client_id] = 0;}

    std::thread([this, client_id]() {
    this->db.markMessagesAsRead(client_id);
}).detach(); 
});
    svr.Get(R"(/recent/(\d+))", [&](const httplib::Request &req, httplib::Response &res)
            {
    int client_id = stoi(req.matches[1]);
    json messages = json::array();

    try {
        // Check if cache already has this client
        if (!c.hasClient(client_id)) {
            // First-time fetch: get from DB
            auto dbResult = db.getRecentMessages(client_id, 2);

            for (auto row : dbResult) {
                Message msg;
                msg.senderId = int(row[1]);
                msg.text = row[3].get<string>();
                msg.timestamp = row[4].get<string>();
                msg.fromDB = true; // mark as DB-fetched

                // Insert into cache
                c.insertMessage(client_id, msg);
            }
        }

        // Serve messages from cache
        messages = c.readRecentMessages(client_id);

        res.status = 200;
        res.set_content(messages.dump(), "application/json");
    } catch (...) {
        res.status = 500;
        res.set_content(R"({"error":"Failed to fetch recent messages"})", "application/json");
    } });

    svr.Get(R"(/history/(\d+))", [&](const httplib::Request &req, httplib::Response &res)
            {
        int client_id = stoi(req.matches[1]);
        auto result = db.getHistory(client_id);
        json messages = json::array();

        for(auto row : result) {
                messages.push_back({
                    {"id", int(row[0])},
                    {"sender_id", int(row[1])},
                    {"receiver_id", int(row[2])},
                    {"msg", (string)row[3]},
                    {"created_at", row[4].get<string>()}
                });
        }

        res.status = 200;
        res.set_content(messages.dump(), "application/json");
        std::thread([this, client_id]()
                    {   c.removeClient(client_id);
                        db.markMessagesAsRead(client_id); })
            .detach();
    });

    // Delete all messages for client
    svr.Delete(R"(/message/(\d+))", [&](const httplib::Request &req, httplib::Response &res)
               {
        int client_id = stoi(req.matches[1]);

        db.deleteMessagesByReceiver(client_id);
        c.removeClient(client_id);

        res.status = 200;
        res.set_content(R"({"status":"Messages deleted"})", "application/json"); });

    svr.Delete(R"(/deactivate/(\d+))", [&](const httplib::Request &req, httplib::Response &res)
               {
        int client_id = stoi(req.matches[1]);

        db.deactivateClient(client_id);

        res.status = 200;
        res.set_content(R"({"status":"Client deactivated "})", "application/json"); });

    svr.listen("0.0.0.0", 8080);
}
