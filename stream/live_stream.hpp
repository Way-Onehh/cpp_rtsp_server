#pragma  once
#include <protocol/rtcp.h>
#include <memory>
#include <stream/stream.hpp>
#include <utility/timer_threadpool.hpp>

class live_stream : public stream
{ 
    std::chrono::steady_clock::time_point  start_time_;
    uint32_t timer,time_base_,frame_szie_,ssrc_;
    timer_threadpool * pool_;

    std::pair<std::shared_ptr<uint8_t[]>,size_t> packet_;
    bool start_ = false;
public:
    void init(threadpool *pool,std::string path,uint32_t ssrc,uint32_t time_base,uint32_t frame_size,std::function<void(std::string)> finish_slot)
    {
        path_ = path;
        on_finish.connect(finish_slot);
    }
    
    //开始函数
    void start(std::chrono::steady_clock::time_point start_time =  std::chrono::steady_clock::now())
    {   
        if( !on_finish )
        {
            throw  std::runtime_error("信号未就绪");
        }

        if(!start_)
        {
            start_ = true;
            start_time_ = start_time;
            make_rtcp({},std::enable_shared_from_this<stream>::shared_from_this());
        }
    }

    void make_rtcp(std::pair<std::shared_ptr<uint8_t[]>,size_t> packet,std::shared_ptr<stream> _this)
    {
        on_rtcp.emit(packet);
    }

    void input(std::pair<std::shared_ptr<uint8_t[]>,size_t> packet,std::shared_ptr<stream> _this) 
    {
        packet_ = packet;
        update(enable_shared_from_this<stream>::shared_from_this());
    }

    void update(std::shared_ptr<stream> _this) 
    {   
        on_send.emit(packet_);
    }
};
