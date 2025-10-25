#include "server.h"
#include <iostream>

using namespace std;

Server::Server(const string &host,
               int port,
               const string &user,
               const string &password,
               const string &db_name)
    : db(host, port, user, password, db_name) {}

void Server::start()
{
    cout << "Server started...\n";

    int choice = 0;
    while (choice != 3)
    {
        cout << "\n1. Insert Message\n2. Show Messages\n3. Exit\nChoice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1)
        {
            int sender, receiver;
            string msg;

            cout << "Sender ID: ";
            cin >> sender;
            cout << "Receiver ID: ";
            cin >> receiver;
            cin.ignore();
            cout << "Message: ";
            getline(cin, msg);

            db.insertMessage(sender, receiver, msg);
        }
        else if (choice == 2)
        {
            auto res = db.getAllMessages();
            for (auto row : res)
            {
                cout << "ID: " << row[0]
                     << ", Sender: " << row[1]
                     << ", Receiver: " << row[2]
                     << ", Message: " << row[3]
                     << ", Timestamp: " << row[4].get<string>() << "\n";
            }
        }
    }

    cout << "Server shutting down.\n";
}
