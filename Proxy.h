#ifndef OSI_LAB_31_PROXY_H
#define OSI_LAB_31_PROXY_H

#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <iostream>
#include <bitset>
#include <csignal>
#include "Handlers/ConnectionHandler.h"
#include "Cache/Cache.h"

class Proxy {
private:
    sockaddr_in address{};
    int socketDesc;     // сокет сервера
    std::vector<pollfd> connections{};
    std::map<int, ConnectionHandler *> handlers;
    bool proxyOn = false;
    Cache *cache;

    void disconnectHandler(int _sock);

public:
    Cache *getCache() { return cache; };

    void addNewHandler(int fd, ConnectionHandler *handler);

    explicit Proxy(int port);

    ConnectionHandler *getHandlerByFd(int fd);

    ~Proxy();

    int createProxySocket(int port);

    bool initProxySocket();

    void addNewConnection(int socket, short event);

    void run();

    void stopProxy();

    void addEvent(int fd, short event);

    void deleteEvent(int fd, short event);

    void makeNewServer(const std::vector<int> &observers);

    void setPollOutEventToObservers(const std::vector<int> &observers);
};


#endif //OSI_LAB_31_PROXY_H
