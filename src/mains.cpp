#include "server.h"
#include "dotenv.h"
#include <iostream>

using namespace std;

int main()
{

    dotenv::init("../.env"); //binary file is in the build folder , and .env is stored in the root folder
    string host = getenv("DB_HOST"); 
    int port = atoi(getenv("DB_PORT")) ;
    string user = getenv("DB_USER") ;
    string password = getenv("DB_PASSWORD") ;
    string db_name = getenv("DB_NAME") ;

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
