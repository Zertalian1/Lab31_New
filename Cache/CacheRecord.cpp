#include "CacheRecord.h"
#include <stdexcept>



void CacheRecord::setFullyCached() {
    this->is_fully_cached = true;
}

int CacheRecord::getObserverCount(){
    return observers.size();
}

void CacheRecord::addObserver(int socket) {
    observers.push_back(socket);
    if (is_fully_cached) {
        setReadyForRead();
    }
}

void CacheRecord::removeObserver(int socket) {
    for (auto iter = observers.begin(); iter != observers.end(); ++iter) {
        if ((*iter) == socket) {
            observers.erase(iter);
            break;
        }
    }
}

void CacheRecord::write(const std::string &str) {// хз как сделать лучше мммммм
    try {
        data.append(str);
    }
    catch (std::bad_alloc &a) {
        std::cerr <<"Proxy ran out of memory. Shutting down...";
        cache->setRanOutOfMemory();
        return;
    }
    setReadyForRead();
}

std::string CacheRecord::read(size_t start, size_t length) const {
    if (data.size() - start < length) {
        return data.substr(start, data.size() - start);
    }
    return data.substr(start, length);
}

CacheRecord::~CacheRecord() {
    observers.clear();
    std::string().swap(data);
}

CacheRecord::CacheRecord(Cache* cache) {
    this->cache = cache;
}

void CacheRecord::deleteRecord(const std::string& url) {
    cache->deleteRecord(url);

}