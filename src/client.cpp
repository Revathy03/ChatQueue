#include "client.h"
#include "httplib.h"
#include "json.hpp"
#include <iostream>

//"httplib.h" → C++ HTTP library for sending GET, PUT, DELETE requests.
//"json.hpp" → JSON library for working with JSON objects.

using namespace std;
using json = nlohmann::json;
// defines json as a shorthand for nlohmann::json.

Client::Client(const string &url) : serverUrl(url) {} //using member initializer list, I can assign directly

void Client::sendMessage(int sender, int receiver, const string &msg)
{
    httplib::Client cli(serverUrl.c_str()); // creates an HTTP client pointing to serverUrl., I acn use httplib::Client cli("127.0.0.1", 8080);
    json body = {                           // Creating a json object
        {"sender_id", sender},
        {"receiver_id", receiver},
        {"msg", msg}};

    auto res = cli.Post("/message", body.dump(), "application/json"); // body.dump() → converts the JSON object into a string to send over HTTP.
    if (res)                                                         // application/json- tells what kind of data is passing
        cout << "Server: " << res->body << endl;
    else
        cout << "Failed to send request" << endl;
}

void Client::getMessages(int client_id)
{
    httplib::Client cli(serverUrl.c_str());                             //.c_str() returns a const char* pointing to the internal null-terminated string of the std::string.
    auto res = cli.Get(("/message/" + to_string(client_id)).c_str());

    if (res){
        json messages = json::parse(res->body);
        for (auto &m : messages)
        {
            cout << "Sender: " << m["sender_id"] << "\n"
                 << "Message: " << m["msg"] << "\n"
                 << "Time: " << m["created_at"] << "\n"
                 << "---------------------------\n";
        }
    }
    else
        cout << "Failed to send request" << endl;
}

void Client::getHistory(int client_id)
{
    httplib::Client cli(serverUrl.c_str()); 
    auto res = cli.Get(("/history/" + to_string(client_id)).c_str());

    if (res){
        json messages = json::parse(res->body);
        for (auto &m : messages)
        {
            cout << "Sender: " << m["sender_id"] << "\n"
                 << "Message: " << m["msg"] << "\n"
                 << "Time: " << m["created_at"] << "\n"
                 << "---------------------------\n";
        }
    }
    else
        cout << "Failed to send request" << endl;
}

void Client::deleteMessages(int client_id)
{
    httplib::Client cli(serverUrl.c_str());
    auto res = cli.Delete(("/message/" + to_string(client_id)).c_str());

    if (res)
        cout << "Server: " << res->body << endl;
    else
        cout << "Failed to send request" << endl;
}

