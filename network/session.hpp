#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <sys/socket.h>

#include <protocol/rtp.h>
#include <utility//log.hpp>
#include <utility/threadpool.hpp>
#include <network/udpchannel.hpp>
#include <packetizer/rtp_packetizer.h>
#include <packetizer/H264_packetizer.hpp>
#include <packetizer/H264_frame_generator.hpp>
#include <packetizer/AAC_frame_generator.hpp>
#include <packetizer/AAC_packetizer.hpp>

class session
{
public:
    enum Status
    {
        INIT,
        SETUP,
        PLAYING,
        TEARDOWN,
        WAIT,
    };
    void setup0(threadpool & pools, int fd , sockaddr addr,std::filesystem::path path0,uint32_t ssrc0,uint32_t dtime0)
    {
        pools_ = & pools;
        if(ch0 ==nullptr)
        {
            ch0.reset(new udpchannel);
            ch0->set(fd,addr);
        }
        H264_rtp_packetizer.frame_generator.open(path0);
        H264_rtp_packetizer.packetizer.set(ssrc0,dtime0);
        status = SETUP;
        
    }

    void setup1(threadpool & pools, int fd , sockaddr addr,std::filesystem::path path1,uint32_t ssrc1,uint32_t dtime1)
    {
        pools_ = & pools;
        if(ch1 ==nullptr)
        {
            ch1.reset(new udpchannel);
            ch1->set(fd,addr);
        }
        AAC_rtp_packetizer.frame_generator.open(path1);
        AAC_rtp_packetizer.packetizer.set(ssrc1,dtime1);
        status = SETUP;
    }
    
    void setteardownf(std::function<void()> f)
    {   
        this->teardownf = f;
    }

    void play()
    {   
        if(status == SETUP) status = PLAYING;
        auto sendpacket = [this]()
        {
            bool flag = 0;
            auto bytes = 0;
            uint8_t buf[UDPMAX];
            while (status == PLAYING) {
                {
                    std::lock_guard lg(mt);
                    flag = bytes = H264_rtp_packetizer.next_packet(buf,UDPMAX);
                    if(bytes >0)
                    { 
                        ch0->send(buf, bytes);
                    }
                    rtp_header* h264_header = reinterpret_cast<rtp_header*>(buf); 

                    if(h264_header->marker)
                    {   
                        for (size_t i = 0; i<3; i++) {
                            bytes = AAC_rtp_packetizer.next_packet(buf,UDPMAX); 
                            if(bytes >0)
                            { 
                                ch1->send(buf, bytes);
                            }
                        }
                    }
                }
                
                if(!flag && !bytes && teardownf)
                {
                     status = TEARDOWN;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000/25));
            }
        };
        //线程池
        pools_->submit(sendpacket);
    }

    void teardown()
    {
        std::lock_guard lg(mt);
        status = TEARDOWN;
    }

public:
    int id = -1;
    Status status = INIT;
    std::shared_ptr<channel> ch0;
    std::shared_ptr<channel> ch1;
    std::function<void()> teardownf;
    rtp_packetizer<H264_frame_generator,H264_packetizer> H264_rtp_packetizer;
    rtp_packetizer<AAC_frame_generator,AAC_packetizer>   AAC_rtp_packetizer;
    std::mutex mt;
    threadpool * pools_;
private:
};