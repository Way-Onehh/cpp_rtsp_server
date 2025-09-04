#pragma once
#include <stdint.h>
#pragma pack(1)
/*
    NAL类型：
        0	没有定义
        1~23	NAL单元 单个NAL单元包
        24	STAP - A 单一时间的组合包
        25	STAP - B 单一时间的组合包
        26	MTAP16 多个时间的组合包
        27	MTAP24 多个时间的组合包
        28	FU - A 分片的单元
        29	FU - B 分片的单元
        30~31	没有定义

    RTP封装常用的就是单一封包和分片封包模式，组合基本不用。
    RTP序号每发送一个包就 + 1 ，同一个NALU的分片的RTP包时间戳不变。
*/

// NAL header 
struct nal_header {
    uint8_t nal_unit_type       :5; // NAL类型 
    uint8_t nal_ref_idc         :2; // 重要性
    uint8_t forbidden_bit       :1; // 必须为0 
};

#pragma pack()