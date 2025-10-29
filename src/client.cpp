#include "client.h"
#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <fstream>

//"httplib.h" → C++ HTTP library for sending GET, PUT, DELETE requests.
//"json.hpp" → JSON library for working with JSON objects.

using namespace std;
using json = nlohmann::json;
// defines json as a shorthand for nlohmann::json.

Client::Client(const string &url) : serverUrl(url)
{

    // Load existing IDs from file
    set<int> existing_ids;
    {
        ifstream infile("../data/client_id.txt");
        int id;
        while (infile >> id)
        {
            existing_ids.insert(id);
        }
    }

    // Register on the server
    httplib::Client cli(serverUrl.c_str());
    json body = {};
    auto res = cli.Post("/register", body.dump(), "application/json");

    if (res)
    {
        cout << "Server: " << res->body << endl;

        json client = json::parse(res->body);
        client_id = client["client_id"];
        cout << "My id: " << client_id << endl;

        // Append only if not present already
        if (existing_ids.find(client_id) == existing_ids.end())
        {
            ofstream outfile("../data/client_id.txt", ios::app);
            outfile << client_id << endl;
            cout << "Client ID saved to file" << endl;
        }
        else
        {
            cout << "Client ID already exists in file" << endl;
        }
    }
    else
    {
        cout << "Failed to send request" << endl;
        client_id = -1; // register failed
    }
}

//using member initializer list, I can assign directly

Client::~Client(){
    httplib::Client cli(serverUrl.c_str());
    auto res = cli.Delete(("/deactivate/" + to_string(client_id)).c_str()); //server will update client table - set to inactive
    if (res)
        cout << "Server: " << res->body << endl;
    else
        cout << "Failed to send request" << endl;

}

int Client::getRandomidFromFile()
{
    std::ifstream file("../data/client_id.txt");
    if (!file.is_open())
    {
        std::cerr << "File not found!\n";
        return -1;
    }

    int lines = 0;
    std::string temp;

    while (std::getline(file, temp))
    {
        lines++;
    }

    if (lines == 0)
        return -1;

    std::srand(std::time(nullptr));
    int randomLine = std::rand() % lines; // pick a line

    file.clear();
    file.seekg(0);

    for (int i = 0; i < randomLine; i++)
    {
        std::getline(file, temp);
    }
    file.close();
    return std::stoi(temp);
}

void Client::sendMessage(const string &msg)
{
    httplib::Client cli(serverUrl.c_str()); // creates an HTTP client pointing to serverUrl., I acn use httplib::Client cli("127.0.0.1", 8080);

    int receiver = client_id;
    while(receiver==client_id){
        receiver = getRandomidFromFile();
        cout << "Receiver id : " << receiver << endl;
    }

    json body = {                           // Creating a json object
        {"sender_id", client_id},
        {"receiver_id", receiver},
        {"msg", msg}};

    auto res = cli.Post("/message", body.dump(), "application/json"); // body.dump() → converts the JSON object into a string to send over HTTP.
    if (res)                                                         // application/json- tells what kind of data is passing
        {
            cout << "Server: " << res->body << endl;
        }
    else
        cout << "Failed to send request" << endl;
}

void Client::getMessages()
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

void Client::getHistory()
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

void Client::deleteMessages()
{
    httplib::Client cli(serverUrl.c_str());
    auto res = cli.Delete(("/message/" + to_string(client_id)).c_str());

    if (res)
        cout << "Server: " << res->body << endl;
    else
        cout << "Failed to send request" << endl;
}

