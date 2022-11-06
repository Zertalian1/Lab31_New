#include "Proxy.h"
#include "Handlers/ClientHandler.h"

bool interrupted = false;

void interruptProxy(int signal){
    interrupted = true;
}

int Proxy::createProxySocket(int port) {
    this->address.sin_port = htons(port);   /* номер порта */
    this->address.sin_family = AF_INET;     /* семейство адресов */
    this->address.sin_addr.s_addr = htonl(INADDR_ANY);  /* адрес IPv4 */
    return socket(AF_INET, SOCK_STREAM, 0);
}

void Proxy::stopProxy(){
    this->proxyOn = false;
}


bool Proxy::initProxySocket() {
    // Связь сокета с адресом
    if (bind(socketDesc, (sockaddr *) &address, sizeof(address)) == -1) {
        std::cerr << "CacheProxy :: Socket bind failed. Shutting down." << '\n';
        return false;
    }
    // Разрешить асинхронный режим работы
    if (fcntl(socketDesc, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "CacheProxy :: Failed to set non-blocking flag to socketDesc. Shutting down." << '\n';
        return false;
    }

    // Слушаем сокет на подключения
    if (listen(socketDesc, 510) == -1) {
        std::cerr << "CacheProxy :: listen(2) failed. Shutting down." << '\n';
        return false;
    }

    return true;
}

void Proxy::addNewConnection(int socket, short event) {
    pollfd fd{};
    fd.fd = socket;
    fd.events = event;
    fd.revents = 0;
    connections.push_back(fd);
}



ConnectionHandler* Proxy::getHandlerByFd(int fd){
    ConnectionHandler* handler;
    try {
        handler = handlers.at(fd);
    } catch (std::out_of_range &e) {
        return nullptr;
    }
    return handler;
}

void Proxy::disconnectHandler(int socket) {
    //std::cerr << "Closing #" << std::to_string(socket);

    for (auto iter = connections.begin(); iter != connections.end(); ++iter) {
        if ((*iter).fd == socket) {
            connections.erase(iter);
            break;
        }
    }

    auto handler = handlers.at(socket);
    handlers.erase(socket);
    close(socket);
    delete handler;
}

void Proxy::deleteEvent(int fd, short event) {

    for (auto &connection : connections) {
        if (connection.fd == fd) {
            connection.events &= ~event;
            break;
        }
    }

}

void Proxy::run() {
    int selectedDescNum;
    int timeOut = 1000;

    while (proxyOn && !interrupted && !cache->ranOutOfMemory()) {
        //std::cout << handlers.size() << '\n';
        setPollOutEventToObservers(cache->getReadyObservers());
        //cache->deleteDeadRecords();
        // ждём до 100 секунд
        if ((selectedDescNum = poll(connections.data(), connections.size(), timeOut)) == -1) {
            std::cerr << "Proxy: run poll internal error";
            return;

        }
        // проверяем успешность вызова
        if (selectedDescNum > 0) {
            // Кто-то пытается подключиться к прокси серверу
            if (connections[0].revents == POLLIN) {
                // обнаружили событие, обнулим revents чтобы можно было переиспользовать структуру
                connections[0].revents = 0;
                // обработка входных данных от sock1
                int newClientFD;
                if ((newClientFD = accept(socketDesc, nullptr, nullptr)) == -1) {
                    std::cerr << "Proxy: Failed to accept new connection" << '\n';
                    continue;
                }
                // Добавляем мониторинг клиента
                addNewConnection(newClientFD, POLLIN | POLLHUP);
                // Обрабатываем(отрабатываем) клиента
                auto *client = (ConnectionHandler *)new ClientHandler(newClientFD, this);
                // добавляем клиента в список клиентов
                addNewHandler(newClientFD, client);
                std::cout << "Proxy: Accepted new connection from client : " << std::to_string(newClientFD) << '\n';
            }

            //
            for (int i = 1; i < connections.size(); ++i) {
                short eventCount = connections[i].revents;
                int socket = (int)connections[i].fd;
                if (eventCount > 0) {
                    // Проверка, как там клиент
                    ConnectionHandler* handler = handlers.at(socket);
                    //std::cout << socket << " action " << eventCount << '\n';
                    if (!(handler->handle(connections[i].revents))) {
                        disconnectHandler(socket);
                        //std::cerr << "Proxy: Closed connection with : " << std::to_string(connections[i].fd) << '\n';
                        //std::cout<<"close connection" << std::endl;
                        //connections[i].revents = 0;
                        i--;
                        continue;
                    }

                    connections[i].revents = 0;

                }
            }
            cache->deleteDeadRecords();
        }
    }
}

void Proxy::addNewHandler(int fd, ConnectionHandler *handler) {
    handlers.insert(std::make_pair(fd, handler));
}

Proxy::~Proxy() {
    for (auto handler : handlers) {
        disconnectHandler(handler.first);
    }
    close(socketDesc);
    delete cache;
}

void Proxy::makeNewServer(const std::vector<int>& observers) {
    for (auto observer : observers) {
        addEvent(observer, POLLOUT);
        auto* client = dynamic_cast<ClientHandler *>(getHandlerByFd(observer));
        if (client != nullptr && client->tryMakeFirstWriter()){
            return;
        }
    }
}

void Proxy::setPollOutEventToObservers(const std::vector<int>& observers){
    for (auto observer : observers){
        ConnectionHandler* handler = handlers.find(observer)->second;
        if(handler->getReadElements() < handler->getCacheRecord()->getDataSize()) {
            addEvent(observer, POLLOUT);
        }
    }
    cache->clearReadyObservers();
}

void Proxy::addEvent(int fd, short event) {
    for (auto &connection : connections) {
        if (connection.fd == fd) {
            connection.events |= event;
            break;
        }
    }

}


Proxy::Proxy(int port) {
    this->socketDesc = createProxySocket(port);
    if (socketDesc == -1) {
        std::cerr << "CacheProxy :: Socket creation failed. Shutting down." << '\n';
        return;
    }

    if (!initProxySocket()) {
        std::cerr << "CacheProxy :: Shutting down due to an internal error." << '\n';
        return;
    }
    // Добавляем мониторинг сервера
    addNewConnection(socketDesc, POLLIN | POLLHUP);
    this->proxyOn = true;
    this->cache = new Cache();
    //memset(&address, 0, sizeof(address));
    // настраиваем прерывание
    sigset(SIGTERM|SIGSEGV, interruptProxy); //SIGINT

}

