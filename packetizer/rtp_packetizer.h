#pragma once
#include <cstring>
#include <memory>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>
#include <protocol/rtp.h>

template<typename T>
concept is_frame_generator_t = requires(T t) {
    {t()}->std::same_as<std::pair<std::shared_ptr<uint8_t[]>,size_t>>;      //提取帧数据
    {bool(t)}->std::same_as<bool>;                                          //判断有帧数据
};


template<typename T>
concept is_packetizer_t = requires(T t, std::pair<std::shared_ptr<uint8_t[]>,size_t> pair) {
    {t = std::move(pair) } -> std::same_as<void>;                           //添加新的数据帧
    {t()} -> std::same_as<std::tuple<std::shared_ptr<uint8_t[]>, size_t,bool>>;    //读取rtp数据包
    {bool(t)}->std::same_as<bool>;                                          //判断有剩余数据
    {t.get_rtcp_args()} ->   std::same_as<std::tuple<
      typename std::__decay_and_strip<int>::__type,
      typename std::__decay_and_strip<unsigned int &>::__type>>;                                       
};


template<is_frame_generator_t frame_generator_t,is_packetizer_t packetizer_t >
class rtp_packetizer
{
public:
    /**
    * @return read packet size 
    */
    auto __read__() -> std::tuple<std::shared_ptr<uint8_t[]>, size_t,bool>  
    {
        bool packetizer_has_data        = bool(packetizer);
        bool frame_generator_has_data   = bool(frame_generator);
        // 数据填充逻辑
        if     (! packetizer_has_data &&  frame_generator_has_data) packetizer = std::move(frame_generator());
        else if(! packetizer_has_data && !frame_generator_has_data) return {{},ENDCODE,0};
        return std::move(packetizer());
    }


    auto read(uint8_t * rtp_data,size_t size) -> size_t
    {   
        auto packet = std::move(__read__());
        if(get<1>(packet) == ENDCODE)  return ENDCODE;
        size_t bytes_to_copy = std::min(size, get<1>(packet)); 
        memcpy(rtp_data, get<0>(packet).get(),  bytes_to_copy);
        return bytes_to_copy;
    }

    //减少中间复制
    auto next_packet()
    {   
        return std::move(__read__());
    }

    
    auto get_rtcp_args()
    {
        return  packetizer.get_rtcp_args();
    }
    packetizer_t packetizer                     ;
    frame_generator_t frame_generator           ;
private:
};