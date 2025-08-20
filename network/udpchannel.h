#pragma once
#include <cstdint>
#include <sys/socket.h>
#include <network/channel.h>
#define UDPMAX 1472

class udpchannel : public channel
{
public:
    void set(int fd , void * ) override;
    
    ssize_t send(uint8_t *buf,size_t n) override;

    void close() override;
public:
    int fd_;
    sockaddr addr_;
};