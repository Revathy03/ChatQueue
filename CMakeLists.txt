cmake_minimum_required(VERSION 3.10)
project(ChatQueue)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# Add source files
#Server executables
set(SOURCES1
    src/mains.cpp
    src/server.cpp
    src/dbconnection.cpp
)

# Create executable for server
add_executable(server ${SOURCES1})

#Client executables
set(SOURCES2
    src/mainc.cpp
    src/client.cpp

)
# Create executable for client
add_executable(client ${SOURCES2})


# Link MySQL Connector/C++ library
#Tells server where to find MySQL header files.
target_include_directories(server PRIVATE /opt/homebrew/include)
#Tells linker where to find MySQL library binaries.
target_link_directories(server PRIVATE /opt/homebrew/lib)
# Links the MySQL X DevAPI library to the server
target_link_libraries(server mysqlcppconnx)  
