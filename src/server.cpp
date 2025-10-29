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
    : db(host, port, user, password, db_name) {} //As the server starts it connects to db, calling the constructor of DBconnection

void Server::start()
{
    httplib::Server svr; //creating a server object

    cout << "REST Server running on http://localhost:8080" << endl;

    // Insert message
    //[&]	Captures variables by reference from outer scope (so you can use db inside)
    //const httplib::Request &req	Contains everything sent by the client: body, headers, URL parameters
    //httplib::Response &res	You fill this to send data back: status code, response JSON, etc.

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
             });

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

            db.insertMessage(body["sender_id"], body["receiver_id"], body["msg"]);
            res.status = 200;
            res.set_content(R"({"status":"Message stored"})", "application/json"); // Without R, you have to escape every ".
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid request"})", "application/json");
        } });

    // Get messages for client
    /*
    \d+ - Capture a number from the URL
    matches[0]	The whole matched URL (/message/42)
    matches[1]	The first captured group → 42
    */
    svr.Get(R"(/message/(\d+))", [&](const httplib::Request &req, httplib::Response &res) 
            {
        int client_id = stoi(req.matches[1]);
        auto result = db.getMessages(client_id);
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
        res.set_content(messages.dump(), "application/json"); });

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
        res.set_content(messages.dump(), "application/json"); });

    // Delete all messages for client
    svr.Delete(R"(/message/(\d+))", [&](const httplib::Request &req, httplib::Response &res) 
               {
        int client_id = stoi(req.matches[1]);

        db.deleteMessagesByReceiver(client_id);

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
