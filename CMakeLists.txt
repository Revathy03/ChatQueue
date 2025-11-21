cmake_minimum_required(VERSION 3.10)
project(ChatQueue)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include project headers
include_directories(include)

# Include MySQL Connector/C++ 8 headers
include_directories(/usr/local/mysql-connector/include)

# Server source files
set(SOURCES1
    src/mains.cpp
    src/server.cpp
    src/dbconnection.cpp
    src/cache.cpp
)

# Client source files
set(SOURCES2
    src/mainc.cpp
    src/client.cpp
)

# Create executables
add_executable(server ${SOURCES1})
add_executable(client ${SOURCES2})

# # Link MySQL Connector/C++ 8 library
# link_directories(/usr/local/mysql-connector/lib64)
# For X DevAPI (mysqlx/xdevapi.h)
target_link_libraries(server /usr/local/mysql-connector/lib64/libmysqlcppconnx.so)
target_link_libraries(client /usr/local/mysql-connector/lib64/libmysqlcppconnx.so)
target_link_libraries(server /usr/local/mysql-connector/lib64/libmysqlcppconn.so)
target_link_libraries(client /usr/local/mysql-connector/lib64/libmysqlcppconn.so)

