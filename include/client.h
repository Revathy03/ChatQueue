#ifndef CLIENT_H
#define CLIENT_H
// Header guards, They prevent double inclusion of this header file.
#include <string>

    class Client
{
private:
    std::string serverUrl;

public:
    Client(const std::string &url);
    void sendMessage(int sender, int receiver, const std::string &msg);
    void getMessages(int client_id);
    void deleteMessages(int client_id);
};

#endif
