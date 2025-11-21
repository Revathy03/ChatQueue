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
    // std::string host = dotenv::get("DB_HOST");
    // int port = std::stoi(dotenv::get("DB_PORT"));
    // std::string user = dotenv::get("DB_USER");
    // std::string password = dotenv::get("DB_PASSWORD");
    // std::string db_name = dotenv::get("DB_NAME");
    // string host = "localhost";
    // int port = 33060;
    // string user = "root";
    // string password = "NewPassword123!";
    // string db_name = "decs_project";

    try
    {
        Server server(host, port, user, password, db_name);
        server.start();
    }
    catch (const std::exception &ex)
    {
        cerr << "Error: " << ex.what() << endl;
    }
    catch (...)
    {
        cerr << "Error: unknown exception\n";
    }

    return 0;
}
