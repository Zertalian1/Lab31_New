#include "ServerHandler.h"

ServerHandler::ServerHandler(int socket, CacheRecord *record){
    this->serverSocket = socket;
    this->cacheRecord = record;
}

ServerHandler::ServerHandler(int socket){
    this->serverSocket = socket;
}

void ServerHandler::setCacheRecord(CacheRecord *record){
    this->cacheRecord = record;
}

//POLLIN - есть данные для чтения.
//POLLHUP - Устройство было отключено, или канал или FIFO были закрыты последним процессом, который открыл его для записи.
bool ServerHandler::handle(int event) {
    if (event & POLLIN) {
        if (!receive()) {
            return false;
        }
    }
    if (event == (POLLIN | POLLHUP)) {
        if (!receive()) {
            return false;
        }
    }
    return true;
}

bool ServerHandler::receive() {
    char buffer[BUF_SIZE];

    ssize_t len = recv(serverSocket, buffer, BUF_SIZE, 0);
    //std::cout << "resp" << '\n' << buffer << '\n';
    if (len > 0) {
        cacheRecord->write(std::string(buffer, len));
    }

    if (len == -1) {
        std::cerr << "Failed to read data from server";
        cacheRecord->setBroken();
        return false;
    }

    if (len == 0) {
        //std::cout << "Hi" << '\n';
        //std::cerr << "Server#" + std::to_string(serverSocket) + " done writing. Closing connection";
        cacheRecord->setFullyCached();
        return false;
    }

    return true;
}

ServerHandler::~ServerHandler() {
    close(serverSocket);
}
