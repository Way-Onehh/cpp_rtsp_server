#include <chrono>
#include <cmath>
#include <cstdint>
#include <protocol/rtcp.h>
#include <utility/log.hpp>
// 获取当前的NTP时间戳（64位）
uint64_t get_ntp_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto since_epoch = now.time_since_epoch(); 
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    auto fraction = since_epoch - secs;
    
    // NTP时间戳是从1900年开始的秒数
    constexpr uint64_t ntp_epoch_offset = 2208988800ULL; // 1970-1900 in seconds
    uint64_t ntp_sec = secs.count()  + ntp_epoch_offset;
    uint64_t ntp_frac = (fraction.count()  * 0x100000000LL) / 1000000000LL;
    
    return (ntp_sec << 32) | ntp_frac;
}

uint32_t calculate_stream_timestamps( std::chrono::steady_clock::time_point stream_start_time,int time_base)
{
    auto t= std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - stream_start_time).count() ;
    auto st =  t/1000*time_base;
    return st;
}

//ssrc start_time time_base frame_rate packet_count octet_count
std::pair<std::shared_ptr<uint8_t[]>, size_t> create_rtcp_sr
(
    uint32_t ssrc, std::chrono::steady_clock::time_point  start_time,uint32_t time_base,uint32_t packet_count_,uint32_t octet_count_
)  
{   
    auto packet_size = sizeof(rtcp_header)+sizeof(sender_info);
    auto packet = std::make_shared<uint8_t[]>(packet_size);
    uint64_t ntp_timestamp = get_ntp_timestamp();
    uint32_t ntp_sec = static_cast<uint32_t>(ntp_timestamp >> 32);
    uint32_t ntp_frac = static_cast<uint32_t>(ntp_timestamp & 0xFFFFFFFF);

    auto header = reinterpret_cast<rtcp_header *>(packet.get());
    header->version = 2;                          // 版本2
    header->padding = 0;                          // 无填充
    header->report_count    = 0;                  // 报告块数量
    header->packet_type     = 200;                // SR包类型
    // 长度字段表示整个 RTCP 报文（包括头部）的长度减 1，单位是32位字（4字节）。
    header->length = htons(packet_size/4 - 1);
    header->ssrc   = htonl(ssrc);
    
    // 2. 填充发送方信息
    auto sender_info = reinterpret_cast<struct sender_info *>(packet.get() + sizeof(rtcp_header) );
    sender_info->ntp_sec        = htonl(ntp_sec);
    sender_info->ntp_frac       = htonl(ntp_frac);
    sender_info->rtp_timestamp  = htonl(calculate_stream_timestamps(start_time,time_base)) ;
    sender_info->packet_count   = htonl(packet_count_);
    sender_info->octet_count    = htonl(octet_count_);
 
    return std::make_pair(packet,packet_size);
}




