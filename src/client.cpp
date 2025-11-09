#include "client.h"
#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <chrono> 

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
    auto start = chrono::high_resolution_clock::now();
    auto res = cli.Post("/register", body.dump(), "application/json");
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();

    if (res)
    {
        cout << "Server: " << res->body << endl;

        json client = json::parse(res->body);
        client_id = client["client_id"];
        cout << "My id: " << client_id << endl;

        // Append only if not present already
        if (existing_ids.find(client_id) == existing_ids.end())
        {
            ofstream outfile("../data/client_id.txt", ios::app); //Open file in append mode
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
  //  cout << "Registration response time: " << duration << " ms" << endl;
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
    ifstream file("../data/client_id.txt");
    if (!file.is_open())
    {
        cout << "File not found!\n";
        return -1;
    }

    vector<int> ids;
    string line;
    while (getline(file, line))
    {
        try
        {
            if (!line.empty())
                ids.push_back(stoi(line));
        }
        catch (...)
        {
            cout << "Invalid ID in file: " << line << endl;
        }
    }
    file.close();

    if (ids.size() <= 1) // only me or none
        return -1;

    srand(time(nullptr));

    int id;
    do
    {
        id = ids[rand() % ids.size()];
    } while (id == client_id);

    return id;
}

void Client::sendMessage(const string &msg)
{
    httplib::Client cli(serverUrl.c_str()); // creates an HTTP client pointing to serverUrl., I acn use httplib::Client cli("127.0.0.1", 8080);

    int receiver = client_id;
    while(receiver==client_id){
        receiver = getRandomidFromFile();
        if (receiver == -1)
        {
            cout << "No valid receiver found. Try running another client.\n";
            return;
        }

        cout << "Receiver id : " << receiver << endl;
    }

    json body = {                           // Creating a json object
        {"sender_id", client_id},
        {"receiver_id", receiver},
        {"msg", msg}};
    auto start = chrono::high_resolution_clock::now();
    auto res = cli.Post("/message", body.dump(), "application/json"); // body.dump() → converts the JSON object into a string to send over HTTP.
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    if (res)                                                         // application/json- tells what kind of data is passing
        {
            cout << "Server: " << res->body << endl;
        }
    else
        cout << "Failed to send request" << endl;
    //cout << "Send message response time: " << duration << " ms" << endl;
}

void Client::getMessages()
{
    httplib::Client cli(serverUrl.c_str());                             //.c_str() returns a const char* pointing to the internal null-terminated string of the std::string.
    auto start = chrono::high_resolution_clock::now();
    auto res = cli.Get(("/message/" + to_string(client_id)).c_str());
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    if (res)
    {
        json messages = json::parse(res->body);
        for (auto &m : messages)
        {
            cout << "Sender: " << m["sender_id"] << "\n"
                 << "Message: " << m["msg"] << "\n"
                 << "Timestamp: " << m["created_at"] << "\n"
            << "---------------------------\n";
        }
    }
    else
        cout << "Failed to send request" << endl;
    cout << "Fetch messages response time: " << duration << " micro sec" << endl;
}

void Client::getHistory()
{
    httplib::Client cli(serverUrl.c_str());
    auto start = chrono::high_resolution_clock::now();
    auto res = cli.Get(("/history/" + to_string(client_id)).c_str());
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    if (res)
    {
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
    cout << "History fetch response time: " << duration << " micro sec" << endl;
}

void Client::deleteMessages()
{
    httplib::Client cli(serverUrl.c_str());
    auto start = chrono::high_resolution_clock::now();
    auto res = cli.Delete(("/message/" + to_string(client_id)).c_str());
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    if (res)
        cout << "Server: " << res->body << endl;
    else
        cout << "Failed to send request" << endl;
    cout << "Delete message response time: " << duration << " micro sec" << endl;
}

