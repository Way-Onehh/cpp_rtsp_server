#pragma once
#include <stdint.h>
#define MAX_RTP_PKT_SIZE 1400
#define NALU_START_CODE 0x00000001
#define NALU_START_CODE_SHORT 0x000001 

#pragma pack(1)
// RTP header
struct rtp_header {
    uint8_t csrc_count          :4; // cc计数器
    uint8_t extension           :1; // 扩展位
    uint8_t padding             :1; // 填充位
    uint8_t version             :2; // rtp版本
    uint8_t payload_type        :7; // 载荷类型
    uint8_t marker              :1; // 标记位一帧的结束
    uint16_t sequence_number    ;   // 序列号
    uint32_t timestamp          ;   // 时间戳
    uint32_t ssrc               ;   // 同步信源标识符
};
#pragma pack()

