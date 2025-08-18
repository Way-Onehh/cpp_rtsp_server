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
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        auto bytes = sendto(fd_,buf,n,0,&addr_,socklen);
        //DLOG(RTP,"send %d",bytes);
        return bytes;
    }
public:
    int fd_;
    sockaddr addr_;
};