#pragma once
#include <stdint.h>

using adts_header = uint8_t[7];

// 提取第1字
// 节字段
#define SYNCWORD(h)         (((h[0] << 4) | (h[1] >> 4)) & 0xFFF)  // 12位  左移隐式提升位 16位 右移丢弃
 
// 提取第2字节字段 
#define ID(h)               ((h[1] >> 3) & 0x01)                   // 1位 
#define LAYER(h)            ((h[1] >> 1) & 0x03)                   // 2位 
#define PROTECTION_ABSENT(h)(h[1] & 0x01)                          // 1位
 
// 提取第3字节字段
#define PROFILE(h)          ((h[2] >> 6) & 0x03)                   // 2位
#define SAMPLE_FREQ_IDX(h)  ((h[2] >> 2) & 0x0F)                   // 4位 
#define PRIVATE_BIT(h)      ((h[2] >> 1) & 0x01)                   // 1位
#define CHANNEL_CONFIG_A(h) (h[2] & 0x01)                          // 用于channel_config
 
// 提取第4字节字段
#define CHANNEL_CONFIG_B(h) ((h[3] >> 6) & 0x03)                   // 用于channel_config
#define ORIGINAL_COPY(h)    ((h[3] >> 5) & 0x01)                   // 1位
#define HOME(h)             ((h[3] >> 4) & 0x01)                   // 1位
#define COPYRIGHT_ID_BIT(h) ((h[3] >> 3) & 0x01)                   // 1位
#define COPYRIGHT_ID_START(h)((h[3] >> 2) & 0x01)                  // 1位
#define FRAME_LEN_A(h)      (h[3] & 0x03)                          // 用于frame_length 
 
// 提取第5-6字节字段 
#define FRAME_LEN_B(h)      (h[4])                                 // 用于frame_length
#define FRAME_LEN_C(h)      (h[5] >> 5)                            // 用于frame_length 
#define BUFFER_FULLNESS_A(h)(h[5] & 0x1F)                          // 用于buffer_fullness 
 
// 提取第7字节字段 
#define BUFFER_FULLNESS_B(h)(h[6] >> 2)                            // 用于buffer_fullness
#define NUM_RAW_BLOCKS(h)   (h[6] & 0x03)                          // 2位
 
// 组合多字节字段 
#define CHANNEL_CONFIG(h)   ((CHANNEL_CONFIG_A(h) << 2) | CHANNEL_CONFIG_B(h))
#define FRAME_LENGTH(h)     ((FRAME_LEN_A(h) << 11) | (FRAME_LEN_B(h) << 3) | FRAME_LEN_C(h))
#define BUFFER_FULLNESS(h)  ((BUFFER_FULLNESS_A(h) << 6) | BUFFER_FULLNESS_B(h))


// 设置第1字节字段
#define SET_SYNCWORD_HI(h, val)  (h[0] = ((val >> 4) & 0xFF))
#define SET_SYNCWORD_LO(h, val)  (h[1] |= ((val << 4) & 0xF0))
 
// 设置第2字节字段 
#define SET_ID(h, val)           (h[1] = (h[1] & 0xF7) | ((val & 0x01) << 3))
#define SET_LAYER(h, val)        (h[1] = (h[1] & 0xF9) | ((val & 0x03) << 1))
#define SET_PROTECTION_ABSENT(h, val) (h[1] = (h[1] & 0xFE) | (val & 0x01))
 
// 设置第3字节字段
#define SET_PROFILE(h, val)      (h[2] = (h[2] & 0x3F) | ((val & 0x03) << 6))
#define SET_SAMPLE_FREQ_IDX(h, val) (h[2] = (h[2] & 0xC3) | ((val & 0x0F) << 2))
#define SET_PRIVATE_BIT(h, val)  (h[2] = (h[2] & 0xFD) | ((val & 0x01) << 1))
#define SET_CHANNEL_CONFIG_A(h, val) (h[2] = (h[2] & 0xFE) | (val & 0x01))
 
// 设置第4字节字段
#define SET_CHANNEL_CONFIG_B(h, val) (h[3] = (h[3] & 0x3F) | ((val & 0x03) << 6))
#define SET_ORIGINAL_COPY(h, val) (h[3] = (h[3] & 0xDF) | ((val & 0x01) << 5))
#define SET_HOME(h, val)         (h[3] = (h[3] & 0xEF) | ((val & 0x01) << 4))
#define SET_COPYRIGHT_ID_BIT(h, val) (h[3] = (h[3] & 0xF7) | ((val & 0x01) << 3))
#define SET_COPYRIGHT_ID_START(h, val) (h[3] = (h[3] & 0xFB) | ((val & 0x01) << 2))
#define SET_FRAME_LEN_A(h, val)  (h[3] = (h[3] & 0xFC) | (val & 0x03))
 
// 设置第5-6字节字段
#define SET_FRAME_LEN_B(h, val)  (h[4] = ((val & 0xFF)))
#define SET_FRAME_LEN_C(h, val)  (h[5] = (h[5] & 0x1F) | ((val & 0x07) << 5))
#define SET_BUFFER_FULLNESS_A(h, val) (h[5] = (h[5] & 0xE0) | (val & 0x1F))
 
// 设置第7字节字段 
#define SET_BUFFER_FULLNESS_B(h, val) (h[6] = (h[6] & 0x03) | ((val & 0x3F) << 2))
#define SET_NUM_RAW_BLOCKS(h, val) (h[6] = (h[6] & 0xFC) | (val & 0x03))
 
// 设置组合字段的便捷宏
#define SET_CHANNEL_CONFIG(h, val) do { \
    SET_CHANNEL_CONFIG_A(h, (val >> 2) & 0x01); \
    SET_CHANNEL_CONFIG_B(h, val & 0x03); \
} while(0)
 
#define SET_FRAME_LENGTH(h, val) do { \
    SET_FRAME_LEN_A(h, (val >> 11) & 0x03); \
    SET_FRAME_LEN_B(h, (val >> 3) & 0xFF); \
    SET_FRAME_LEN_C(h, val & 0x07); \
} while(0)
 
#define SET_BUFFER_FULLNESS(h, val) do { \
    SET_BUFFER_FULLNESS_A(h, (val >> 6) & 0x1F); \
    SET_BUFFER_FULLNESS_B(h, val & 0x3F); \
} while(0)
 
// 初始化ADTS头部的宏 固定0xFFF
#define SET_SYNCWORD(h) do {memset(h, 0, sizeof(adts_header));SET_SYNCWORD_HI(h, 0xFFF);SET_SYNCWORD_LO(h, 0xFFF);} while(0)