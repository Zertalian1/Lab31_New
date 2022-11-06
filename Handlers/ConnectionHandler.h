#ifndef OSI_LAB_31_CONNECTIONHANDLER_H
#define OSI_LAB_31_CONNECTIONHANDLER_H

#include "../Cache/CacheRecord.h"

#define BUF_SIZE 1024

class ConnectionHandler {
public:
    virtual ~ConnectionHandler() = default;

    virtual void deleteCache() = 0;

    virtual CacheRecord* getCacheRecord() = 0;

    virtual bool handle(int event) = 0;

    virtual size_t getReadElements() = 0;
private:
};


#endif //OSI_LAB_31_CONNECTIONHANDLER_H
