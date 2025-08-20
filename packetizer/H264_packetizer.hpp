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