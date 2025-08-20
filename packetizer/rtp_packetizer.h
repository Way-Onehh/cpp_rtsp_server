#pragma once
#include <cstring>
#include <memory>
#include <concepts>
#include <cstddef>
#include <cstdint>
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
    {t()} -> std::same_as<std::pair<std::shared_ptr<uint8_t[]>,size_t>>;    //读取rtp数据包
    {bool(t)}->std::same_as<bool>;                                          //判断有剩余数据
};


template<is_frame_generator_t frame_generator_t,is_packetizer_t packetizer_t >
class rtp_packetizer
{
public:
    /**
    * @return read packet size 
    */
    auto next_packet(uint8_t * rtp_data,size_t size) -> size_t
    {   
        std::pair<std::shared_ptr<uint8_t[]>,size_t> packet;
        bool packetizer_has_data        = bool(packetizer);
        bool frame_generator_has_data   = bool(frame_generator);
        // 数据填充逻辑
        if     (! packetizer_has_data &&  frame_generator_has_data) packetizer = std::move(frame_generator());
        else if(! packetizer_has_data && !frame_generator_has_data)                            return ENDCODE;
        
        packet = std::move(packetizer());
        size_t bytes_to_copy = std::min(size, packet.second); 
        memcpy(rtp_data, packet.first.get(),  bytes_to_copy);
        return bytes_to_copy;
    }
private:
public:
    packetizer_t packetizer                     ;
    frame_generator_t frame_generator           ;
};