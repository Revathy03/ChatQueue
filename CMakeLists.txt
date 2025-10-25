cmake_minimum_required(VERSION 3.10)
project(DECS_PROJECT)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# Add source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/dbconnection.cpp
)

# Create executable
add_executable(decsp ${SOURCES})

# Link MySQL Connector/C++ library
target_include_directories(decsp PRIVATE /opt/homebrew/include)
target_link_directories(decsp PRIVATE /opt/homebrew/lib)
target_link_libraries(decsp mysqlcppconnx)  
