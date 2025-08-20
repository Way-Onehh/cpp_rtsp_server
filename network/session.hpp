#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <sys/socket.h>

#include <protocol/rtp.h>
#include <utility//log.hpp>
#include <utility/threadpool.hpp>
#include <network/udpchannel.h>
#include <packetizer/rtp_packetizer.h>
#include <packetizer/H264_packetizer.hpp>
#include <packetizer/H264_frame_generator.hpp>
#include <packetizer/AAC_frame_generator.hpp>
#include <packetizer/AAC_packetizer.hpp>

class session : public std::enable_shared_from_this<session>
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
        setup_flag0 = 1;
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
        setup_flag1 = 1;
    }
    
    void play()
    {
        if(status == SETUP) status = PLAYING;

        //线程池
        if(setup_flag0){pools_->submit(std::bind(&session::play_video,this,std::enable_shared_from_this<session>::shared_from_this()));}
        
        if(setup_flag1){pools_->submit(std::bind(&session::play_audio,this,std::enable_shared_from_this<session>::shared_from_this()));}
    }

    //有权函数
    void play_video(std::shared_ptr<session> _this)
    {
        auto bytes = 0;
        uint8_t buf[UDPMAX];

        {
            bytes = _this->H264_rtp_packetizer.next_packet(buf,UDPMAX);
            if(bytes >0)
            {
                _this->ch0->send(buf, bytes);
            }
            rtp_header* h264_header = reinterpret_cast<rtp_header*>(buf);
            if(h264_header->marker)
            {
            std::this_thread::sleep_for(std::chrono::milliseconds(23));
            }
        }

        if(bytes == ENDCODE)
        {
            _this->setup_flag0 = 0;
            if(!_this->setup_flag0 && !_this->setup_flag1)
            {
                _this->teardown();
            }
        }
        if(status == PLAYING)
        pools_->submit(std::bind(&session::play_video,this,std::enable_shared_from_this<session>::shared_from_this()));
    }

    //有权函数
    void play_audio(std::shared_ptr<session> _this)
    {
        auto bytes = 0;
        uint8_t buf[UDPMAX];

        {
            bytes = _this->AAC_rtp_packetizer.next_packet(buf,UDPMAX);
            if(bytes >0)
            {
                _this->ch1->send(buf, bytes);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(23));
        }

        if(bytes == ENDCODE)
        {
            _this->setup_flag1 = 0;
            if(!_this->setup_flag0 && !_this->setup_flag1)
            {
                _this->teardown();
            }
        }
        if(status == PLAYING)
        pools_->submit(std::bind(&session::play_audio,this,std::enable_shared_from_this<session>::shared_from_this()));
    }
    void teardown()
    {
        this->status = TEARDOWN;
    }

public:
    int id = -1;
    Status status = INIT;
    std::shared_ptr<channel> ch0;
    std::shared_ptr<channel> ch1;
    rtp_packetizer<H264_frame_generator,H264_packetizer> H264_rtp_packetizer;
    rtp_packetizer<AAC_frame_generator,AAC_packetizer>   AAC_rtp_packetizer;
    std::mutex mt_video;
    std::mutex mt_audio;
    threadpool * pools_;

    bool setup_flag0 = 0;
    bool setup_flag1 = 0;
    int times=0;
private:
};