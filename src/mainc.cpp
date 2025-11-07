#include "client.h"
#include <iostream>
#include <string>
#include <random>

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
            int receiver;
            // string msg="";
            // //cout << "Enter Receiver ID: ";
            // //cin >> receiver;
            // cin.ignore();
            // cout << "Enter Message: ";
            // while(msg==""){
            //     getline(cin, msg);
            //     if(msg=="")
            //         cout << "Please enter your message: ";
            // }
            static const string charset = "abcdefghijklmnopqrstuvwxyz ";
            random_device rd;        //random number generator
            mt19937 gen(rd());       // Mersenne Twister PRNG produce deterministic number, rd() gives random seed Mersenne Twister PRNG
            uniform_int_distribution<> len_dist(3, 10); // random length between 3 and 10
            uniform_int_distribution<> char_dist(0, charset.size() - 1);

            int msg_len = len_dist(gen);
            string msg;
            for (int i = 0; i < msg_len; ++i)
            {
                msg += charset[char_dist(gen)];
            }

            cout << "Auto-generated Message: " << msg << endl;
            client.sendMessage(msg);
        }
        else if (choice == 2)
        {
            client.getMessages();
        }
        else if (choice == 3)
        {

            client.getHistory();
        }
        else if (choice == 4)
        {

            client.deleteMessages();
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