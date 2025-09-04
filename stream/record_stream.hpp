#pragma  once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <utility>
#include <stdexcept>

#include <utility/log.hpp>
#include <utility/threadpool.hpp>
#include <utility/timer_threadpool.hpp>
#include <utility/timer_recoder.hpp>
#include <packetizer/H264_packetizer.hpp>
#include <packetizer/H264_frame_generator.hpp>
#include <packetizer/AAC_frame_generator.hpp>
#include <packetizer/AAC_packetizer.hpp>
#include <packetizer/rtp_packetizer.h>
#include <protocol/rtcp.h>
#include <stream/stream.hpp>

using record_video = rtp_packetizer<H264_frame_generator,H264_packetizer>  ;
using record_audio = rtp_packetizer<AAC_frame_generator,AAC_packetizer>    ;

template <typename  rtp_packetizer_t>
class record_stream : public stream 
{
    rtp_packetizer_t rtp_packetizer;
    std::chrono::steady_clock::time_point  start_time_;
    uint32_t timer,time_base_,frame_szie_,ssrc_;
    timer_threadpool * pool_;
    std::string path_;
    bool start_ = false;

public:
    ~record_stream()
    {
        DLOG(STREAM,"remove stream %s", path_.c_str());
    }
    
    void init(threadpool *pool,std::string path,uint32_t ssrc,uint32_t time_base,uint32_t frame_size,std::function<void(std::string)> finish_slot)
    {   
        pool_ =dynamic_cast<timer_threadpool*>(pool);
        if(!pool_) std::runtime_error("检查 timer_threadpool 定义");
        time_base_ = time_base;
        frame_szie_ = frame_size;
        path_ = path;
        timer = frame_size  * 1000 / time_base ;
        ssrc_ = ssrc; 
        if(!rtp_packetizer.frame_generator.open(path)) throw std::runtime_error("无法打开文件");
        rtp_packetizer.packetizer.set(ssrc,frame_size);
        on_finish.connect(finish_slot);
    };

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
            update(std::enable_shared_from_this<stream>::shared_from_this());
            make_rtcp({},std::enable_shared_from_this<stream>::shared_from_this());
        }
    }

    //调度函数
    void update(std::shared_ptr<stream> _this)
    {
        auto streambuf = std::move(rtp_packetizer.next_packet());
        auto last_result = get<1>(streambuf);
        auto is_frame = get<2>(streambuf);
        if(last_result <= 0 || last_result == ENDCODE) 
        {
            on_finish.emit(path_);
            start_ = 0;
            DLOG(STREAM,"%s(%s)%s","stream ",path_.c_str()," dont play");  return;
        }

        on_send.emit(std::make_pair( get<0>(streambuf), get<1>(streambuf)));
        
        if(is_frame)
        {
            pool_->schedule(timer,std::bind(&record_stream<rtp_packetizer_t>::update,this,std::enable_shared_from_this<stream>::shared_from_this())); 
        }    
        else
        {
            pool_->submit(std::bind(&record_stream<rtp_packetizer_t>::update,this,std::enable_shared_from_this<stream>::shared_from_this()));
        }
    }

    //调度函数
    void make_rtcp(std::pair<std::shared_ptr<uint8_t[]>,size_t> packet,std::shared_ptr<stream> _this)
    {
        if(start_)
        {pool_->schedule(5000,std::bind(&record_stream<rtp_packetizer_t>::make_rtcp,this,packet,std::enable_shared_from_this<stream>::shared_from_this()));}
        auto args           =   rtp_packetizer.get_rtcp_args();
        auto rtcp_packet    =   create_rtcp_sr(ssrc_,start_time_ ,time_base_,get<0>(args),get<1>(args));
        {
            on_rtcp.emit(rtcp_packet);
        }
    }

private:
    //slot by read
    void input(std::pair<std::shared_ptr<uint8_t[]>,size_t>,std::shared_ptr<stream> _this) 
    {
        throw std::runtime_error("这个流类型不需要使用这个函数");
        return;
    };
};