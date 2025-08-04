#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <ctime>
#include <netinet/in.h>

// RTP头部结构（网络字节序）
struct RtpHeader {
    uint8_t version_padding_ext;
    uint8_t marker_payload;
    uint16_t seq_num;
    uint32_t timestamp;
    uint32_t ssrc;
};
 
// 生成RTP测试文件
void generateRtpFile(const std::string& filename, int packet_count = 10) {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
 
    // 固定参数配置 
    const uint8_t version = 2;       // RTP版本 
    const uint8_t payload_type = 96; // H.264负载类型 
    const uint32_t ssrc = 0x12345678;
    const int payload_size = 1024;   // 每个包的负载大小 
 
    uint16_t seq_num = 0;
    uint32_t timestamp = static_cast<uint32_t>(time(nullptr));
 
    for (int i = 0; i < packet_count; ++i) {
        // 构造RTP头部 
        RtpHeader header;
        header.version_padding_ext  = (version << 6); // 版本2，无填充和扩展
        header.marker_payload  = (i == packet_count - 1) ? (1 << 7) | payload_type : payload_type;
        header.seq_num  = htons(seq_num++);
        header.timestamp  = htonl(timestamp + i * 90000 / 30); // 假设30fps 
        header.ssrc  = htonl(ssrc);
 
        // 写入头部 
        outfile.write(reinterpret_cast<const  char*>(&header), sizeof(RtpHeader));
 
        // 生成随机负载数据 
        std::vector<uint8_t> payload(payload_size);
        for (auto& byte : payload) {
            byte = rand() % 256; // 简单随机数据 
        }
 
        // 写入负载 
        outfile.write(reinterpret_cast<const  char*>(payload.data()),  payload_size);
 
        // 每5个包打印进度 
        if (i % 5 == 0) {
            std::cout << "Generated packet " << i << "/" << packet_count 
                      << " (seq=" << seq_num-1 << ")" << std::endl;
        }
    }
 
    std::cout << "Successfully generated " << packet_count 
              << " RTP packets to " << filename << std::endl;
}
 
int main() {
    // 配置参数
    const std::string output_file = "rtp_test_data.bin"; 
    const int packet_count = 20; // 生成包数量
    // 生成文件 
    generateRtpFile(output_file, packet_count);
    return 0;
}