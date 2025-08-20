#pragma once
#include <cstdint>
#include <sys/socket.h>
#include <network/channel.h>


class tcpchannel : public channel
{
public:
    void set(int fd , void *) override;
    ssize_t send(uint8_t *buf,size_t n) override;
    void close() override;
public:
    int fd_;
    int ch_n;
    int is_close = 0;
};
