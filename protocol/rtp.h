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

// FU indicator 
struct fu_indicator {
    uint8_t type                :5; // FU-A类型(28)
    uint8_t nal_ref_idc         :2; // 重要性 
    uint8_t forbidden_bit       :1; // 必须为0 
};

// FU header
struct fu_header {
    uint8_t nal_type            :5; // 原始NAL类型
    uint8_t reserved_bit        :1; // 保留位(必须为0)
    uint8_t end_bit             :1; // 结束位
    uint8_t start_bit           :1; // 起始位
};


// AU

#pragma pack()

