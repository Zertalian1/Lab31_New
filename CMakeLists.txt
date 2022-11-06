cmake_minimum_required(VERSION 3.23)
project(Lab31_New)

set(CMAKE_CXX_STANDARD 14)

add_executable(Lab31_New main.cpp Cache/Cache.cpp Cache/Cache.h Cache/CacheRecord.cpp Cache/CacheRecord.h Handlers/ClientHandler.cpp Handlers/ClientHandler.h Handlers/ConnectionHandler.h Handlers/ServerHandler.cpp Handlers/ServerHandler.h Proxy.cpp Proxy.h)
