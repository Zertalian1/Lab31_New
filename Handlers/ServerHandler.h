//
// Created by zerta on 19.10.2022.
//

#ifndef OSI_LAB_31_SERVERHANDLER_H
#define OSI_LAB_31_SERVERHANDLER_H

#include <iostream>
#include <netdb.h>
#include <codecvt>
#include <locale>
#include <netinet/tcp.h>
#include <bitset>
#include <poll.h>
#include <unistd.h>
#include "../Cache/CacheRecord.h"
#include "ConnectionHandler.h"

class ServerHandler : public ConnectionHandler {
private:
    int serverSocket;
    CacheRecord *cacheRecord = nullptr;

public:

    void deleteCache() override{};

    CacheRecord* getCacheRecord() override { return  cacheRecord;}

    ServerHandler(int socket, CacheRecord *record);

    explicit ServerHandler(int socket);

    void setCacheRecord(CacheRecord *record);

    bool handle(int event) override;

    int getSocket() const { return serverSocket; };

    ~ServerHandler() override;

    size_t getReadElements() override {return 0; };

    bool receive();
};


#endif //OSI_LAB_31_SERVERHANDLER_H
