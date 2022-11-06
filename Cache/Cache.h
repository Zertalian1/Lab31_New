#ifndef OSI_LAB_31_CACHE_H
#define OSI_LAB_31_CACHE_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "CacheRecord.h"

class CacheRecord;

class Cache {
private:
    std::map<std::string, CacheRecord* > cache;
    bool ran_out_of_memory = false;
    std::vector<int> readyObservers;

public:
    void deleteRecord(const std::string& url);

    Cache() = default;

    ~Cache();

    void deleteDeadRecords();

    bool isCached(const std::string &url);

    CacheRecord *subscribe(const std::string &url, int socket);

    void unsubscribe(const std::string &url, int socket);

    CacheRecord *addRecord(const std::string &url);

    bool isFullyCached(const std::string &url);

    bool ranOutOfMemory() const { return ran_out_of_memory; };

    void setRanOutOfMemory() { ran_out_of_memory = true; };

    std::vector<int> getReadyObservers();

    void clearReadyObservers();
};


#endif //OSI_LAB_31_CACHE_H
