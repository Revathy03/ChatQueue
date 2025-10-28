#include "client.h"
#include <iostream>

using namespace std;
int main()
{
    Client client("http://localhost:8080");

    while (true)
    {
        cout << "\n========== ChatQueue Client ==========\n";
        cout << "1. Send Message\n";
        cout << "2. View My Messages\n";
        cout << "3. View My History\n";
        cout << "4. Delete My Messages\n";
        cout << "5. Exit\n";
        cout << "Choose: ";

        int choice;
        cin >> choice;

        if (choice == 1)
        {
            int sender, receiver;
            string msg="";
            cout << "Enter Sender ID: ";
            cin >> sender;
            cout << "Enter Receiver ID: ";
            cin >> receiver;
            cin.ignore();
            cout << "Enter Message: ";
            while(msg==""){
                getline(cin, msg);
                if(msg=="")
                    cout << "Please enter your message: ";
            }
            client.sendMessage(sender, receiver, msg);
        }
        else if (choice == 2)
        {
            int id;
            cout << "Enter your Client ID: ";
            cin >> id;
            client.getMessages(id);
        }
        else if (choice == 3)
        {
            int id;
            cout << "Enter your Client ID: ";
            cin >> id;
            client.getHistory(id);
        }
        else if (choice == 4)
        {
            int id;
            cout << "Enter your Client ID: ";
            cin >> id;
            client.deleteMessages(id);
        }
        else if (choice == 5)
        {
            cout << "Exiting client...\n";
            break;
        }
        else
        {
            cout << "Invalid choice!\n";
        }
    }

    return 0;
}