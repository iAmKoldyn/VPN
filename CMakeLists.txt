cmake_minimum_required(VERSION 3.23)
project(dbvpn)

set(CMAKE_CXX_STANDARD 14)

include_directories(client)
include_directories(server)

add_executable(dbvpn
        client/dbvpnclient.cpp
        client/dbvpnclient.h
        client/main.cpp
        server/dbvpnserver.cpp
        server/dbvpnserver.h
        server/main.cpp)
