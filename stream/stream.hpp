#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility/signal.hpp>
#include <utility/threadpool.hpp>
#include <protocol/sdp.hpp>

class stream :public std::enable_shared_from_this<stream>
{
public:
    signal<std::pair<std::shared_ptr<uint8_t[]>, size_t>> on_send;
    signal<std::pair<std::shared_ptr<uint8_t[]>, size_t>> on_rtcp;
    signal<std::string> on_finish;
    std::string path_;
public:
    virtual void init(threadpool *pool,std::string path,uint32_t ssrc,uint32_t time_base,uint32_t frame_size,std::function<void(std::string)> finish_slot)=0;
    
    virtual void start(std::chrono::steady_clock::time_point start_time =  std::chrono::steady_clock::now()) = 0;

    //schedule
    virtual void input(std::pair<std::shared_ptr<uint8_t[]>,size_t>,std::shared_ptr<stream> _this) =0;

    //schedule
    virtual void update(std::shared_ptr<stream> _this)=0;
    
    virtual void make_rtcp(std::pair<std::shared_ptr<uint8_t[]>,size_t> packet,std::shared_ptr<stream> _this)=0;
    virtual ~stream() = default;
};
