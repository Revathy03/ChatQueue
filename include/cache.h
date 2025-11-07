#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <string>
#include <list>
#include <deque>
#include <vector>
#include <mutex>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

struct Message
{
    int senderId;
    string text;
    string timestamp;
};

struct ClientNode
{
    int clientId;
    deque<Message> messages;
};

class Cache
{
private:
    int maxClients;
    int maxMsgsPerClient;
    mutable mutex mtx;

    list<ClientNode> clientList;
    unordered_map<int, list<ClientNode>::iterator> clientMap;

    void moveToFront(int clientId);
    void evictLeastUsedClient();

public:
    Cache(int maxClients = 100, int maxMsgs = 50);
    void printCache();

    void insertMessage(int clientId, const Message &msg);

    json readUnreadMessages(int clientId); // JSON return

    bool hasClient(int clientId);
    void removeClient(int clientId);
};

#endif
