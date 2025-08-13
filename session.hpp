#pragma once
#include "threadpool.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <log.hpp>
#include <ratio>
#include <rtp.h>
#include <sys/socket.h>
#include <rtp_packetizer.h>
#include <H264_packetizer.hpp>
#include <H264_frame_generator.hpp>
#include <memory>
#include <thread>


class channel
{
public:
    virtual void set(int fd , sockaddr addr) = 0;
    virtual ssize_t send(uint8_t * buf,size_t n) = 0;
};


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
        DLOG(RTP,"send %d",bytes);
        return bytes;
    }
public:
    int fd_;
    sockaddr addr_;
};

class session
{
public:
    enum Status
    {
        INIT,
        SETUP,
        PLAYING,
        TEARDOWN
    };
    void setup(threadpool & pools, int fd , sockaddr addr,std::filesystem::path path,uint32_t ssrc,uint32_t dtime)
    {
        
        pools_ = & pools;
        ch.reset(new udpchannel);
        ch->set(fd,addr);
        H264_rtp_packetizer.frame_generator.open(path);
        H264_rtp_packetizer.packetizer.set(ssrc,dtime);
        status = SETUP;
    }
    void play()
    {
        if(timce > 1000000) return;
        if(status == SETUP ||  status == PLAYING) 
        {
            status = PLAYING;
            auto sendpacket = [this]()
            {   
                uint8_t buf[UDPMAX];
                auto bytes = H264_rtp_packetizer.next_packet(buf,UDPMAX); 
                if(bytes >0)
                { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    ch->send(buf, bytes);
                    if(this->status == PLAYING)
                    this->pools_->submit(std::bind(&session::play,this));
                }
            };
            this->pools_->submit(sendpacket);
        }
        timce++;
    }

    void teardown()
    {
        status = TEARDOWN;
    }
public:
    Status status = INIT;
    std::shared_ptr<channel> ch;
    rtp_packetizer<H264_frame_generator,H264_packetizer> H264_rtp_packetizer;
    threadpool * pools_;
    int timce = 0;
private:
};