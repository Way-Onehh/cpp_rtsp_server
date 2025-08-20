#include <vector>
#include <cstdint>
#include <arpa/inet.h> // 用于ntohs/ntohl 
 
// RTCP报文头部（通用结构）
struct RtcpHeader {
    uint8_t version;      // 版本（固定为2）
    bool padding;         // 填充位
    uint8_t report_count; // 报告块数量或子类型
    uint8_t packet_type;  // RTCP类型标识 
    uint16_t length;      // 报文长度（以32位字为单位，减1）
    uint32_t ssrc;        // 同步源标识符（部分报文类型使用）
};

// 从网络字节序解析头部 
RtcpHeader Parse(const uint8_t* data) {
    RtcpHeader header;
    uint32_t word = ntohl(*reinterpret_cast<const uint32_t*>(data));
    header.version  = (word >> 30) & 0x03;
    header.padding  = (word >> 29) & 0x01;
    header.report_count  = (word >> 24) & 0x1F;
    header.packet_type  = (word >> 16) & 0xFF;
    header.length  = (word & 0xFFFF) * 4; // 转换为字节长度 
    return header;
}
 
// SR（Sender Report）报文结构
struct RtcpSrPacket {
    RtcpHeader header;
    uint32_t ntp_timestamp_high; // NTP时间戳高位 
    uint32_t ntp_timestamp_low;  // NTP时间戳低位
    uint32_t rtp_timestamp;      // RTP时间戳
    uint32_t packet_count;       // 发送的包总数 
    uint32_t octet_count;        // 发送的总字节数 
    // 可扩展：包含接收报告块（report_count > 0）
};
 
// RR（Receiver Report）报文结构
struct RtcpRrPacket {
    RtcpHeader header;
    // 可扩展：接收报告块（report_count > 0）
};
 
// SDES（Source Description）报文结构 
struct RtcpSdesPacket {
    RtcpHeader header;
    struct SdesItem {
        uint8_t type;   // 项类型（如CNAME=1）
        uint8_t length; // 数据长度
        uint8_t data[]; // 变长数据 
    };
    std::vector<SdesItem> items;
};
 
// BYE（Goodbye）报文结构
struct RtcpByePacket {
    RtcpHeader header;
    std::vector<uint32_t> ssrcs; // 离开的SSRC列表 
};
 
// APP（Application-Defined）报文结构 
struct RtcpAppPacket {
    RtcpHeader header;
    char name[4];       // 应用标识符
    uint8_t subtype;    // 子类型
    uint8_t data[];     // 变长数据 
};
 
