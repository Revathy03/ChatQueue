#include "server.h"
#include <iostream>
#include <cstdlib> // for getenv
using namespace std;

int main()
{
    string host = getenv("DB_HOST") ? getenv("DB_HOST") : "localhost"; //have to setup environment variables
    int port = atoi(getenv("DB_PORT") ? getenv("DB_PORT") : "33060");    
    string user = getenv("DB_USER") ? getenv("DB_USER") : "root"; 
    string password = getenv("DB_PASSWORD") ? getenv("DB_PASSWORD") : "";
    string db_name = getenv("DB_NAME") ? getenv("DB_NAME") : "decs_project"; 

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
