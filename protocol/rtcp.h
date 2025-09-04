#pragma once
#include <cstdint>
#include <arpa/inet.h> 
#include <cstring>
#include <memory>
#include <chrono>

#pragma pack(1)

struct rtcp_header {
    uint8_t padding         :1;         // 填充位
    uint8_t report_count    :5;         // 报告块数量或子类型
    uint8_t version         :2;         // 版本（固定为2）
    uint8_t packet_type;                // RTCP类型标识 
    uint16_t length;                    // 报文长度（以32位字为单位，减1）
    uint32_t ssrc;                      // 同步源标识符（部分报文类型使用）
};

struct sender_info  {
    uint32_t ntp_sec;                   // NTP时间戳高位 
    uint32_t ntp_frac;                  // NTP时间戳低位
    uint32_t rtp_timestamp;             // RTP时间戳
    uint32_t packet_count;              // 发送的包总数 
    uint32_t octet_count;               // 发送的总字节数 
};

#pragma pack()

std::pair<std::shared_ptr<uint8_t[]>, size_t> create_rtcp_sr
(
    uint32_t ssrc, std::chrono::steady_clock::time_point  start_time,uint32_t time_base,uint32_t packet_count_,uint32_t octet_count_
);