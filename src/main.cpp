#include "server.h"
#include <iostream>
#include <cstdlib>   //enviroment variable - for password
using namespace std;

int main()
{
    string host = "localhost";
    int port = 33060;
    string user = "root";
    string password = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "";
    string db_name = "decs_project";

    try
    {
        Server server(host, port, user, password, db_name);
        server.start();
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << endl;
    }

    return 0;
}
