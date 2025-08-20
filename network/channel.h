#pragma  once
#include <cstdint>
#include <sys/socket.h>
class channel
{
public:
    virtual void set(int fd , void * ) = 0;
    virtual ssize_t send(uint8_t * buf,size_t n) = 0;
    virtual void close() = 0;
};
