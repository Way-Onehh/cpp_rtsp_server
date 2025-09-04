#pragma  once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sys/socket.h>
#include <utility/signal.hpp>

class channel
{
public:
    signal<std::pair< std::shared_ptr<uint8_t[]>,size_t>> on_recv;
    signal<int,void *> on_close;
    virtual void set(int fd , void * ) = 0;
    virtual ssize_t send(uint8_t * buf,size_t n) = 0;
    virtual void close() = 0;
};
