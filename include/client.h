#ifndef CLIENT_H
#define CLIENT_H
// Header guards, They prevent double inclusion of this header file.
#include <string>
#include <fstream>

using namespace std;

class Client
{
private:
    string serverUrl;
    int client_id;

public:
    Client(const std::string &url);
    ~Client();
    int getRandomidFromFile();
    void sendMessage(const std::string &msg);
    void getMessages();
    void getHistory();
    void deleteMessages();
};

#endif
