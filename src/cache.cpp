#include "cache.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
using namespace std;

Cache::Cache(int maxClients, int maxMsgs)
    : maxClients(maxClients), maxMsgsPerClient(maxMsgs) {}

void Cache::printCache() //For testing 
{
    lock_guard<mutex> lock(mtx);
    cout << "\n===== CACHE STATE =====\n";
    if (clientList.empty())
    {
        cout << "Cache is empty\n";
        return;
    }

    int pos = 1;
    for (auto &node : clientList)
    {
        cout << pos++ << ". Client " << node.clientId << " → Messages: ";

        if (node.messages.empty())
        {
            cout << "[None]";
        }
        else
        {
            cout << "\n";
            for (auto &m : node.messages)
            {
                cout << "   - From " << m.senderId
                     << ": " << m.text << "\n"
                << ": " << m.timestamp << "\n";
            }
        }
        cout << "\n";
    }

    cout << "=======================\n\n";
}

void Cache::moveToFront(int clientId)
{
    auto it = clientMap[clientId];
    clientList.splice(clientList.begin(), clientList, it);
}

void Cache::evictLeastUsedClient()
{
    auto last = clientList.back();
    clientMap.erase(last.clientId);
    clientList.pop_back();
}

void Cache::insertMessage(int clientId, const Message &msgInput)
{
    lock_guard<mutex> lock(mtx);
    Message msg = msgInput;

    if (msg.timestamp.empty())
    {
        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm local_tm = *localtime(&t);
        stringstream ss;
        ss << put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        msg.timestamp = ss.str();
    }

    if (clientMap.find(clientId) == clientMap.end())
    {
        if (clientList.size() >= maxClients)
            evictLeastUsedClient();

        clientList.push_front({clientId, {}});
        clientMap[clientId] = clientList.begin();
    }

    auto &msgs = clientMap[clientId]->messages;

    if (msgs.size() >= maxMsgsPerClient)
        msgs.pop_back();

    msgs.push_front(msg);
    moveToFront(clientId);

    cout << "Cache: Inserted msg to client " << clientId
         << " @ " << msg.timestamp
         << (msg.fromDB ? " [fromDB]" : " [unread]") << endl;
}

json Cache::readUnreadMessages(int clientId)
{
    lock_guard<mutex> lock(mtx);
    json result = json::array();

    if (!hasClient(clientId))
        return result;

    auto nodeIt = clientMap[clientId];
    auto &msgs = nodeIt->messages;

    auto it = msgs.begin();
    while (it != msgs.end())
    {
        if (!it->fromDB) // only unread messages
        {
            result.push_back({{"sender_id", it->senderId},
                              {"msg", it->text},
                              {"created_at", it->timestamp}});
            it = msgs.erase(it); // remove from cache after reading
        }
        else
        {
            ++it; // keep DB messages
        }
    }

    return result;
}
json Cache::readRecentMessages(int clientId)
{
    lock_guard<mutex> lock(mtx);
    json result = json::array();

    if (!hasClient(clientId))
        return result;

    auto nodeIt = clientMap[clientId];
    auto &msgs = nodeIt->messages;

    for (auto &m : msgs)
    {
        if (m.fromDB)
        {
            result.push_back({{"sender_id", m.senderId},
                              {"msg", m.text},
                              {"created_at", m.timestamp}});
        }
    }
    return result;
}

    bool Cache::hasClient(int clientId)
    {
        return clientMap.count(clientId);
    }

    void Cache::removeClient(int clientId)
    {
        if (!hasClient(clientId))
            return;
        clientList.erase(clientMap[clientId]);
        clientMap.erase(clientId);
    }
