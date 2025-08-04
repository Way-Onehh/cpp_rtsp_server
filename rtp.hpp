#pragma once

struct packet_header
{
    uint8_t version : 2;        // 协议版本（通常为2）
    uint8_t padding : 1;        // 填充位 
    uint8_t extension : 1;      // 扩展头标志 
    uint8_t csrc_count : 4;     // CSRC标识符计数（0-15）
    uint8_t marker : 1;         // 标记位（视频关键帧/音频帧边界）
    uint8_t payload_type : 7;   // 负载类型（如96=H.264）
    uint16_t sequence_number;   // 序列号（防丢包乱序）
    uint32_t timestamp;         // 时间戳（采样率相关）
    uint32_t ssrc;              // 同步信源标识符
};

