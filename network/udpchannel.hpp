#pragma once
#include <cstdint>
#include <thread>
#include <sys/socket.h>

#include <network/channel.h>
class udpchannel : public channel
{
public:
    void set(int fd , sockaddr addr) override
    {
        fd_ = fd;
        addr_ = addr; 
    }
    
    ssize_t send(uint8_t *buf,size_t n) override
    {
        socklen_t socklen = sizeof(sockaddr);
        auto bytes = sendto(fd_,buf,n,0,&addr_,socklen);
        return bytes;
    }
public:
    int fd_;
    sockaddr addr_;
};