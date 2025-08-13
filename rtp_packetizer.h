#pragma once
#include <cstring>
#include <memory>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

template<typename T>
concept is_frame_generator_t = requires(T t) {
    {t()}->std::same_as<std::pair<std::unique_ptr<uint8_t[]>,size_t>>;      //提取帧数据
    {bool(t)}->std::same_as<bool>;                                          //判断有帧数据
};


template<typename T>
concept is_packetizer_t = requires(T t, std::pair<std::unique_ptr<uint8_t[]>,size_t> pair) {
    {t = std::move(pair) } -> std::same_as<void>;                           //添加新的数据帧
    {t()} -> std::same_as<std::pair<std::unique_ptr<uint8_t[]>,size_t>>;    //读取rtp数据包
    {bool(t)}->std::same_as<bool>;                                          //判断有剩余数据
};


template<is_frame_generator_t frame_generator_t,is_packetizer_t packetizer_t >
class rtp_packetizer
{
public:
    /**
    * @return read packet size 
    */
    auto next_packet(uint8_t * rtp_data,size_t size) -> int
    {   
        std::pair<std::unique_ptr<uint8_t[]>,size_t> packet;
        if(!packetizer)
        {
            if(frame_generator) packetizer = std::move(frame_generator());
            else return                                                 0;
        }             
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