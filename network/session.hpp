#pragma once

#include <unistd.h>
#include <memory>
#include <chrono>
#include <cstdint>
#include <functional>
#include <sys/socket.h>
#include <protocol/rtp.h>
#include <protocol/rtcp.h>
#include <utility//log.hpp>
#include <utility/threadpool.hpp>
#include <utility/timer_threadpool.hpp>
#include <stream/stream.hpp>
#include <network/udpchannel.h>
#include <network/server_config.h>
#include <network/stream_server.h>
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
        ANNOUNCING,
        TEARDOWN,
        WAIT,
    };

    enum type
    {
        tcp,
        udp
    };
    
    void init(threadpool *pools_,int fd,int id , type type)
    {   
        this->fd = fd;
        this->id = id;
        this->type_ = type;
        this->last_point = std::chrono::steady_clock::now();
        this->handle_alive(pools_, std::enable_shared_from_this<session>::shared_from_this());
    }

    void keep_alive()
    {
        this->last_point = std::chrono::steady_clock::now();
    }

    void handle_alive(threadpool *pools_,std::shared_ptr<session> _this)
    {
        if(!_this->cheak_alive())
            ON_CLOSE_CLIENT->emit(fd);
        if(status == PLAYING)
        {
            timer_threadpool *t_pools_ = dynamic_cast<timer_threadpool *>(pools_);
            t_pools_->schedule(600000,std::bind(&session::handle_alive,this,pools_,std::enable_shared_from_this<session>::shared_from_this()));
        }
    }

    bool cheak_alive()
    {
        auto cost_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - this->last_point).count();
        return cost_time < 60;
    }

     void setup0(std::shared_ptr<stream> video_stream,std::shared_ptr<channel>  ch_ptr0, std::shared_ptr<channel>  ch_ptr1)
    {
        video_stream_ = video_stream;
        ch0 = ch_ptr0;
        ch1 = ch_ptr1;
        if(status != ANNOUNCING)
        {status = SETUP;}
        setup_flag0 = 1;
    }

    void setup1(std::shared_ptr<stream> audio_stream,std::shared_ptr<channel>  ch_ptr0, std::shared_ptr<channel>  ch_ptr1)
    {
        audio_stream_ = audio_stream;
        ch2 = ch_ptr0;
        ch3 = ch_ptr1;
        if(status != ANNOUNCING)
        {status = SETUP;}
        setup_flag1 = 1;
    }
    void announce0()
    {
        ch0->on_recv.connect(std::bind(&stream::input,video_stream_.lock().get(),std::placeholders::_1,video_stream_.lock()->shared_from_this()));
        ch1->on_recv.connect(std::bind(&stream::make_rtcp,video_stream_.lock().get(),std::placeholders::_1,video_stream_.lock()->shared_from_this()));
    }
    void announce1()
    {
        ch2->on_recv.connect(std::bind(&stream::input,audio_stream_.lock().get(),std::placeholders::_1,audio_stream_.lock()->shared_from_this()));
        ch3->on_recv.connect(std::bind(&stream::make_rtcp,audio_stream_.lock().get(),std::placeholders::_1,audio_stream_.lock()->shared_from_this()));
    }
    void play()
    {
        if(status == SETUP) status = PLAYING;

        //线程池
        if(setup_flag0)
        {
            index0 = video_stream_.lock()->on_rtcp.connect(std::bind(&session::send_rtcp0,this,std::placeholders::_1,std::enable_shared_from_this<session>::shared_from_this()));
            video_stream_.lock()->on_send.connect(std::bind(&session::play_video,this,std::placeholders::_1,std::enable_shared_from_this<session>::shared_from_this()));
            video_stream_.lock()->start();
        }
        
        if(setup_flag1)
        {
            index1 = audio_stream_.lock()->on_rtcp.connect(std::bind(&session::send_rtcp1,this,std::placeholders::_1,std::enable_shared_from_this<session>::shared_from_this()));
            audio_stream_.lock()->on_send.connect(std::bind(&session::play_audio,this,std::placeholders::_1,std::enable_shared_from_this<session>::shared_from_this()));
            audio_stream_.lock()->start();
        }
    }

    //有权函数
    void play_video(std::pair<std::shared_ptr<uint8_t[]>, size_t> packet,std::shared_ptr<session> _this)
    {
        _this->ch0->send(packet.first.get(), packet.second);
    }

    //有权函数
    void play_audio(std::pair<std::shared_ptr<uint8_t[]>, size_t> packet,std::shared_ptr<session> _this)
    {
        _this->ch2->send(packet.first.get(), packet.second);
    }

    //有权函数
    void send_rtcp0(std::pair<std::shared_ptr<uint8_t[]>, size_t> packet,std::shared_ptr<session> _this)
    {
        _this->ch1->send(packet.first.get(), packet.second);
        times0++;
    }

    //有权函数
    void send_rtcp1(std::pair<std::shared_ptr<uint8_t[]>, size_t> packet,std::shared_ptr<session> _this)
    {
        _this->ch3->send(packet.first.get(), packet.second);
        times1++;
    }

    void teardown()
    {   
        if(ch0)
        {ch0->close();}
        if(ch2)
        {ch2->close();}
        if(ch1)
        {ch1->close();}
        if(ch3)
        {ch3->close();}
        this->status = TEARDOWN;
    }

public:
    int id = -1;
    int fd = -1;
    
    Status status = INIT;
    type type_ = tcp;

    std::shared_ptr<channel> ch0;
    std::shared_ptr<channel> ch1;
    std::shared_ptr<channel> ch2;
    std::shared_ptr<channel> ch3;
    
    std::weak_ptr<stream> video_stream_;
    std::weak_ptr<stream> audio_stream_;
;
    std::chrono::steady_clock::time_point last_point = std::chrono::steady_clock::now();
    
    uint32_t time_base0_ = 0;
    uint32_t time_base1_ = 0;

    uint32_t frame_rate0_ = 0;
    uint32_t frame_rate1_ = 0;

    int times0 = 0;
    int times1 = 0;

    int index0 = -1;
    int index1 = -1;

    bool setup_flag0 = 0;
    bool setup_flag1 = 0;

private:
};