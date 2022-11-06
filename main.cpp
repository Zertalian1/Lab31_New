#include <iostream>
#include <csignal>
#include "Proxy.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "need port number" << std::endl;
        return -1;
    }

    sigset(SIGPIPE, SIG_IGN);
    sigset(SIGSTOP, SIG_IGN);

    struct sigaction act{};
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, nullptr);

    std::cout << "STARTING PROXY ON PORT " << argv[1]  << " ..."<< std::endl;

    int port = atoi(argv[1]);
    auto* proxy = new Proxy(port);

    proxy->run();

    delete proxy;

    std::cout << "PROXY IS Мёртв" << std::endl;

    return 0;
}
