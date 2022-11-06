#include "ClientHandler.h"

#include <utility>
ClientHandler::ClientHandler(int sock, Proxy *clientProxy){
    this->clientSocket = sock;
    this->proxy = clientProxy;
    this->server = nullptr;
    this->url = "";
    this->host = "";
    this->record = nullptr;
}

void ClientHandler::deleteCache(){
    std::cout << "deleteCache";
    record->deleteRecord(url);
}

//POLLIN - есть данные для чтения.
//POLLHUP - Устройство было отключено, или канал или FIFO были закрыты последним процессом, который открыл его для записи.
//POLLOUT - есть данные для отправки.
//POLLERR - Произошла ошибка
// смотрим со стороны сервера( у сервера есть данные на отправку, у сервера есть данные для чтения)
bool ClientHandler::handle(int event) {

    if (event == (POLLHUP | POLLIN)) {
        return false;
    }

    if(event == (POLLHUP | POLLERR | POLLIN)){
        return false;
    }

    if (event & POLLIN) {
        if (!receive()) {
            return false;
        }
        //proxy->addEvent(clientSocket, POLLOUT);
        return true;
    }



    if (event & POLLOUT) {
        if (!initialized || record->isBroken()) {
            return false;
        }

        if (!firstWriter && readPointer >= record->getDataSize() && !cachingInParallel) {
            //std::cerr << "Caught up with the first writer. Now caching in parallel";
            //std::cout << "??" << '\n';
            cachingInParallel = true;
            return true;
        }
        if (readPointer <= record->getDataSize()) {
            //std::cout << "proxy send data" << "\n";
            std::string buffer = record->read(readPointer, BUF_SIZE);
            ssize_t ret = send(clientSocket, buffer.data(), buffer.size(), 0);
            if (ret == -1) {
                perror("send failed");
            } else {
                readPointer += buffer.size();
            }

            if ((!record->isFullyCached() && cachingInParallel) || buffer.empty()) {
                //std::cout << "asdasdas" << '\n';
                //proxy->getCache()->unsubscribe(url, clientSocket);
                proxy->deleteEvent(clientSocket, POLLOUT);
                //std::cout << "close POLLOUT" << std::endl;
            }
        }

    }
    return true;

}

// "GET http://lib.pushkinskijdom.ru/Default.aspx?tabid=10183 HTTP/1.1\r\nHost: lib.pushkinskijdom.ru\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:103.0) Gecko/20100101 Firefox/103.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\nAccept-Language: en-US,en;q=0.5\r\nAccept-Encoding: gzip, deflate\r\nReferer: http://lib.pushkinskijdom.ru/Default.aspx?tabid=2018\r\nConnection: keep-alive\r\nCookie: .ASPXANONYMOUS=uLMKm8gi2QEkAAAAOGEwYjJhYzItNzVmMS00OTRjLTlmZjMtYjAwNzQ4MTZkYTk40; __utma=261296784.1672019940.1667117300.1667190460.1667197894.4; __utmz=261296784.1667117300.1.1.utmcsr=(direct)|utmccn=(direct)|utmcmd=(none); __utmc=261296784; language=ru-RU\r\nUpgrade-Insecure-Requests: 1\r\n\r\n"
std::string ClientHandler::getPrVersion(std::string in){
    std::string req = std::move(in);
    size_t start = req.find("HTTP/");
    size_t end = req.find("\r\n");
    if(start == std::string::npos || end == std::string::npos) {
        std::cerr <<"parse Host error" << std::endl;
        return "";
    }
    return req.substr(start + 5, end - start - 5);
}

std::string getHost(std::string in){
    std::string req = std::move(in);
    size_t start = req.find("Host:");
    size_t end = req.find("User-Agent:");
    if(start == std::string::npos || end == std::string::npos) {
        std::cerr <<"parse Host error" << std::endl;
        return "";
    }
    return req.substr(start + 6, end - start - 8);
}

std::string ClientHandler::getUrl(std::string in){
    std::string req = std::move(in);
    size_t start = req.find(' ');
    size_t end = req.find("HTTP");
    if(start == std::string::npos || end == std::string::npos) {
        std::cerr <<"parse URL error" << std::endl;
        return "";
    }
    return req.substr(start + 1 , end - start - 2);
}

std::string ClientHandler::getMethod(std::string in){
    std::string req = std::move(in);
    size_t end = req.find(' ');
    if(end == std::string::npos) {
        std::cerr <<"parse URL error" << std::endl;
        return "";
    }
    return req.substr(0, end);
}

bool ClientHandler::RequestParser(){
    prVersion = getPrVersion(request);
    if(prVersion!="1.1" && prVersion!="1.0" ){
        char NOT_ALLOWED[71] = "HTTP/1.0 505 HTTP VERSION NOT SUPPORTED\r\n\r\n HTTP Version Not Supported";
        write(clientSocket, NOT_ALLOWED, 71);
        return false;
    }
    //std::cout << prVersion << '\n';
    std::string HTTPMethod = getMethod(request);
    //std::cout << HTTPMethod << '\n';
    if(HTTPMethod != "GET" && HTTPMethod != "POST"){
        char NOT_ALLOWED[59] = "HTTP/1.0 405 METHOD NOT ALLOWED\r\n\r\n Method Not Allowed";
        write(clientSocket, NOT_ALLOWED, 59);
        return false;
    }
    host = getHost(request);
    url = getUrl(request);
    return true;
}

bool ClientHandler::initConnectionToDest(){
    /*if(!RequestParser()){
        return false;
    }*/
    //std::cout << url << '\n';
    if (!proxy->getCache()->isCached(url) ) {
        if(!becomeFirstWriter()){
            return false;
        }
        //std::cout << url + " new record created" << std::endl;
/*        if(proxy->getCache()->isCached(url) || prVersion == "1.1"){
            record->setDeleteAfterUse();
        }*/
        initialized = true;
        return true; // подключение к серверу, отправка запроса на него
    } else{
        std::cout << "loaded from cache " << url << '\n';
        record = proxy->getCache()->subscribe(url, clientSocket);
        //proxy->addEvent(clientSocket, POLLOUT);
    }
    initialized = true;
}

//"GET http://lib.pushkinskijdom.ru/Default.aspx?tabid=2018 HTTP/1.1\r\nHost: lib.pushkinskijdom.ru\r\n
bool ClientHandler::receive() {
    char buffer[BUFSIZ];

    ssize_t len = recv(clientSocket, buffer, BUFSIZ, 0);
    if (len < 0) {
        std::cerr << "Failed to read data from clientSocket";
        return false;
    }

    if (len == 0) {
        /*//std::cout <<'/n/n/n/n/n/n'<< "Client #" + std::to_string(clientSocket) + " done writing. Closing connection" << '/n/n/n/n';
        proxy->deleteEvent(clientSocket, POLLIN); // last
        std::cout << "close POLLIN"<< std::endl;
        if (firstWriter && record->getObserverCount() > 1) {
            proxy->makeNewServer(record->getObservers());
        }*/
        //std::cout << " uns" << '\n';
        proxy->getCache()->unsubscribe(url, clientSocket);
        return false;
    }

    request.append(buffer, len);

    if(len<BUFSIZ) {        // медленно ыыыыы
        if(record != nullptr){
            record->setFullyCached();
            proxy->getCache()->unsubscribe(url, clientSocket);
        }
        if(!RequestParser()){       // перенести парсинг и создание record сюда для протокола 1.1
            return false;
        }
        if (!initialized) {
            if(!initConnectionToDest()){
                return false;
            }
        } else {
            writeToServer(request); // last
        }
        record = proxy->getCache()->addRecord(url);
        if (record == nullptr) {
            std::cerr << "Failed to allocate new cache record for " + url;
            proxy->stopProxy();
            return false;
        }
        server->setCacheRecord(record);
        proxy->getCache()->subscribe(url, clientSocket);
        readPointer=0;
        request.clear();
    }
    return true;
}

bool ClientHandler::tryMakeFirstWriter() {
    return becomeFirstWriter();
}

bool ClientHandler::becomeFirstWriter() {
    firstWriter = true;
    readPointer = 0;
    int serverSocket = connectToServer(host);

    if (serverSocket == -1) {
        std::cerr << "Cannot connect to: " + host + " closing connection.";
        return false;
    } else {
        createServer(serverSocket);
        /*record = proxy->getCache()->addRecord(url);

        if (record == nullptr) {
            std::cerr << "Failed to allocate new cache record for " + url;
            proxy->stopProxy();
            return false;
        }
        createServer(serverSocket, record);
        proxy->getCache()->subscribe(url, clientSocket);*/

    }

    if (!writeToServer(request)) {
        return false;
    }
    return true;
}

void ClientHandler::setURL(const char *at, size_t len) {
    url.append(at, len);
}

std::string ClientHandler::getLastField() {
    return lastField;
}

void ClientHandler::setLastField(const char *at, size_t len) {
    lastField.append(at, len);
}

int ClientHandler::connectToServer(const std::string& host) {
    /*struct addrinfo hints{0};
    struct addrinfo *result, *rp;
    int sfd, s;

    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    s = getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    struct hostent *hostinfo = gethostbyname(host.data());
    int point=0;
    for(char* addr = (hostinfo->h_addr_list)[point];point < hostinfo->h_length; point++ ){
        printf("%s", addr);
    }
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        std::cout << rp->ai_addr << '\n';
        if (sfd == -1)
            continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;
        close(sfd);
    }
    freeaddrinfo(result);
    if (rp == nullptr){
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }
    return sfd;
*/
    int serverSocket;
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        std::cerr << "Failed to open new socket";
        return -1;
    }

    struct hostent *hostinfo = gethostbyname(host.data());
    if (hostinfo == nullptr) {
        std::cerr << "Unknown host" + host;
        close(serverSocket);
        return -1;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    if ((connect(serverSocket, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn))) == -1) {
        std::cerr << "Cannot connect to" + host;
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

void ClientHandler::resetLastField() {
    lastField = "";
}

void ClientHandler::createServer(int socket) {
    server = new ServerHandler(socket);
    proxy->addNewConnection(socket, (POLLIN | POLLHUP));
    proxy->addNewHandler(socket, server);
}

bool ClientHandler::writeToServer(const std::string &msg) {
    ssize_t len = send(server->getSocket(), msg.data(), msg.size(), 0);

    if (len == -1) {
        std::cerr << "Failed to send data to server";
        record->setBroken();
        return false;
    }
    //std::cout << "req" << '\n' << msg << '\n';
    return true;
}

void ClientHandler::setHost(const char *at, size_t len) {
    host.append(at, len);
}

size_t ClientHandler::getReadElements() {
    return readPointer;
}

// кэш не работает нормально для http 1.1
