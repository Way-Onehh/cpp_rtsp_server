// #pragma once

// #include <memory>
// #include <cstdint>
// #include <algorithm>
// #include <netinet/in.h>

// #include <utility/log.hpp>
// #include <protocol/rtp.h>
// #include <protocol/h264.h>

// #define UDPMAX 1472


// class H264_packetizer {
// private:
//     // 内部状态跟踪 
//     std::shared_ptr<uint8_t[]> current_nal_;
//     size_t nal_size_ = 0;
//     size_t offset_ = 0;
//     uint16_t sequence_number_ = 0;
//     bool has_more_packets_ = false;
//     const size_t mtu_ = 1400;
//     uint32_t ssrc_ = 0;
//     uint32_t timestamp_ = 0;
//     uint32_t dtime_ = 0;
// public: 
//     // 操作符重载：处理NAL单元并返回RTP包 
//     auto set(uint32_t ssrc ,uint32_t dtime)
//     {
//         this->dtime_=dtime;
//         this->ssrc_=ssrc;
//     }

//     //填充数据帧
//     auto operator = (
//         std::pair<std::shared_ptr<uint8_t[]>, size_t> nal_unit) {
//         current_nal_ = std::move(nal_unit.first); 
//         nal_size_ = nal_unit.second; 
//         offset_ = 0;
//         has_more_packets_ = true;
//     }
 
//     // 判断是否还有更多RTP包需要读取 
//     explicit operator bool() const {
//         return has_more_packets_;
//     }
    
//     // 获取下一个RTP包
//     auto operator()() -> std::pair<std::shared_ptr<uint8_t[]>, size_t> {
//         if(*this)
//         {
//             if (nal_size_ <= mtu_ - sizeof(rtp_header) - sizeof(nal_header)) {
//                 // 单包模式
//                 return create_single_packet();
//             } else {
//                 // 分片模式 
//                 return create_fragmented_packet();
//             }
//         }
//         return {};
//     }
// private:
//     // 创建单个NAL单元的RTP包
//     auto create_single_packet() ->std::pair<std::shared_ptr<uint8_t[]>, size_t> {
//         const size_t packet_size = sizeof(rtp_header) + nal_size_;
//         auto packet = std::make_shared<uint8_t[]>(packet_size);
        
//         // 填充RTP头部 
//         rtp_header* header = reinterpret_cast<rtp_header*>(packet.get());
//         header->version = 2;
//         header->padding = 0;
//         header->extension = 0;
//         header->csrc_count = 0;
//         header->marker = 0;
//         header->payload_type = 96;
//         header->sequence_number =htons( sequence_number_);
//         header->ssrc =htonl(this->ssrc_);
//         header->timestamp = htonl(this->timestamp_);
//         // 复制NAL数据
//         std::copy_n(current_nal_.get(), nal_size_, packet.get()  + sizeof(rtp_header));
//         uint8_t naluType = current_nal_.get()[0];
       
//         //sps pps sei 不是视频帧 
//         if ( (naluType & 0x1F) != 7 && (naluType & 0x1F) != 8)
//         {
//             if((naluType & 0x1F) != 6)  header->marker = 1;   
//             this->timestamp_ += this->dtime_; 
//         } 

//         //DLOG(RTP,"maker %d sequence_number %d timestamp %d  packet_size %u", static_cast<bool>(header->marker),static_cast<int>(ntohs(header->sequence_number)),ntohl(header->timestamp),packet_size);
//         has_more_packets_ = false;
//         ++sequence_number_;
//         return {std::move(packet), packet_size};
//     }
 
//     // 创建分片NAL单元的RTP包 
//     auto create_fragmented_packet() ->std::pair<std::shared_ptr<uint8_t[]>, size_t> {
//         const size_t max_payload =  mtu_ - sizeof(rtp_header) - sizeof(nal_header);
//         const size_t payload_size = std::min(max_payload, nal_size_ - offset_);
//         const size_t packet_size = sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header) + payload_size;
        
//         auto packet = std::make_shared<uint8_t[]>(packet_size);
        
//         // 填充RTP头部 
//         rtp_header* header = reinterpret_cast<rtp_header*>(packet.get()); 
//         header->version = 2;
//         header->padding = 0;
//         header->extension = 0;
//         header->csrc_count = 0;
//         header->marker = (offset_ + payload_size >= nal_size_) ? 1 : 0; // 最后一个分片标记结束 
//         header->payload_type = 96;
//         header->sequence_number =htons( sequence_number_);
//         header->timestamp =htonl(this->timestamp_) ;
//         header->ssrc =htonl(this->ssrc_);
//         // 获取原始NAL头
//         nal_header* original_header = reinterpret_cast<nal_header*>(current_nal_.get());
 
//         // 填充FU indicator 
//         fu_indicator* indicator = reinterpret_cast<fu_indicator*>(packet.get()  + sizeof(rtp_header));
//         indicator->forbidden_bit = original_header->forbidden_bit;
//         indicator->nal_ref_idc = original_header->nal_ref_idc;
//         indicator->type = 28; // FU-A类型
 
//         // 填充FU header 
//         fu_header* fu_hdr = reinterpret_cast<fu_header*>(packet.get()  + sizeof(rtp_header) + sizeof(fu_indicator));
//         fu_hdr->start_bit = (offset_ == 0) ? 1 : 0;
//         fu_hdr->end_bit = (offset_ + payload_size >= nal_size_) ? 1 : 0;
//         fu_hdr->reserved_bit = 0;
//         fu_hdr->nal_type = original_header->nal_unit_type;
//         offset_ += (offset_ == 0) ? 1 : 0;
//         // 复制NAL数据
//         std::copy_n(current_nal_.get() + offset_,
//                    payload_size,
//                    packet.get()  + sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header));
       
//         offset_ += payload_size;
//         DLOG(,"%d",payload_size);
//         ++sequence_number_;
//         has_more_packets_ = (offset_ < nal_size_);
//         if(!has_more_packets_) this->timestamp_ += this->dtime_;
//         return {std::move(packet), packet_size};
//     }

// };

#pragma once 
 
#include <cstring>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <netinet/in.h>
#include <utility/log.hpp>
#include <protocol/rtp.h>
#include <protocol/h264.h>
#define MAX_NAL_SIZE (10 * 1024 * 1024) // 最大NAL单元大小10MB 
#define DEFAULT_MTU 1400 
 
class H264_packetizer {
private:
    std::shared_ptr<uint8_t[]> current_nal_;
    size_t nal_size_ = 0;
    size_t offset_ = 0;
    uint16_t sequence_number_ = 0;
    bool has_more_packets_ = false;
    size_t mtu_ = DEFAULT_MTU;
    uint32_t ssrc_ = 0;
    uint32_t timestamp_ = 0;
    uint32_t dtime_ = 0;
 
public:
    void set(uint32_t ssrc, uint32_t dtime) {
        this->dtime_ = dtime;
        this->ssrc_ = ssrc;
    }
 
    void operator=(std::pair<std::shared_ptr<uint8_t[]>, size_t> nal_unit) {
        if (!nal_unit.first  || nal_unit.second  == 0 || nal_unit.second  > MAX_NAL_SIZE) {
            has_more_packets_ = false;
            return;
        }
        
        current_nal_ = std::move(nal_unit.first); 
        nal_size_ = nal_unit.second; 
        offset_ = 0;
        has_more_packets_ = true;
    }
 
    explicit operator bool() const {
        return has_more_packets_ && current_nal_ && nal_size_ > 0;
    }
 
    auto operator()() -> std::pair<std::shared_ptr<uint8_t[]>, size_t> {
        if (!*this) return {nullptr, 0};
 
        // 确保MTU足够容纳基本RTP头 
        size_t effective_mtu = std::max(mtu_, sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header) + 1);
 
        if (nal_size_ <= effective_mtu - sizeof(rtp_header) - sizeof(nal_header)) {
            return create_single_packet();
        } else {
            return create_fragmented_packet();
        }
    }
 
private:
    auto create_single_packet() -> std::pair<std::shared_ptr<uint8_t[]>, size_t> {
        if (!current_nal_ || nal_size_ == 0) return {nullptr, 0};
 
        const size_t packet_size = sizeof(rtp_header) + nal_size_;
        if (packet_size > mtu_ * 2) { // 防止异常大的包
            has_more_packets_ = false;
            return {nullptr, 0};
        }
 
        auto packet = std::shared_ptr<uint8_t[]>(new uint8_t[packet_size]);
 
        // 填充RTP头部
        auto* header = reinterpret_cast<rtp_header*>(packet.get()); 
        memset(header, 0, sizeof(rtp_header));
        header->version = 2;
        header->payload_type = 96;
        header->sequence_number = htons(sequence_number_);
        header->timestamp = htonl(timestamp_);
        header->ssrc = htonl(ssrc_);
 
        // 复制NAL数据 
        std::copy_n(current_nal_.get(), nal_size_, packet.get()  + sizeof(rtp_header));
 
        // 设置marker位 
        uint8_t naluType = current_nal_[0] & 0x1F;
        if (naluType != 7 && naluType != 8 && naluType != 6) { // 非SPS/PPS/SEI帧 
            header->marker = 1;
            timestamp_ += dtime_;
        }
 
        has_more_packets_ = false;
        ++sequence_number_;
        return {std::move(packet), packet_size};
    }
 
    auto create_fragmented_packet() -> std::pair<std::shared_ptr<uint8_t[]>, size_t> {
        if (!current_nal_ || nal_size_ == 0 || offset_ >= nal_size_) {
            return {nullptr, 0};
        }
 
        const size_t header_size = sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header);
        const size_t max_payload = mtu_ - header_size;
        if (max_payload == 0) {
            has_more_packets_ = false;
            return {nullptr, 0};
        }
 
        bool is_first = (offset_ == 0);
        size_t payload_size = std::min(max_payload, nal_size_ - offset_ - (is_first ? 1 : 0));
        if (payload_size == 0) {
            has_more_packets_ = false;
            return {nullptr, 0};
        }
 
        const size_t packet_size = header_size + payload_size;
        auto packet = std::shared_ptr<uint8_t[]>(new uint8_t[packet_size]);
 
        // 填充RTP头部 
        auto* header = reinterpret_cast<rtp_header*>(packet.get()); 
        memset(header, 0, sizeof(rtp_header));
        header->version = 2;
        header->payload_type = 96;
        header->sequence_number = htons(sequence_number_);
        header->timestamp = htonl(timestamp_);
        header->ssrc = htonl(ssrc_);
        header->marker = (offset_ + payload_size + (is_first ? 1 : 0) >= nal_size_) ? 1 : 0;
 
        // 填充FU indicator和header 
        uint8_t* payload = packet.get()  + sizeof(rtp_header);
        payload[0] = (current_nal_[0] & 0xE0) | 28; // FU-A类型
        payload[1] = (current_nal_[0] & 0x1F);      // NAL单元类型
 
        // 设置FU header标志位
        if (is_first) {
            payload[1] |= 0x80; // Start bit
            offset_++; // 跳过NAL头
        }
        if (offset_ + payload_size >= nal_size_) {
            payload[1] |= 0x40; // End bit
        }
 
        // 复制NAL数据
        std::copy_n(current_nal_.get() + offset_, payload_size, payload + 2);
        offset_ += payload_size;
 
        ++sequence_number_;
        has_more_packets_ = (offset_ < nal_size_);
        if (!has_more_packets_) {
            timestamp_ += dtime_;
        }
 
        return {std::move(packet), packet_size};
    }
};