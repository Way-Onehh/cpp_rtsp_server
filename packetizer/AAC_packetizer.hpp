#pragma once

#include <memory>
#include <cstdint>
#include <algorithm>
#include <netinet/in.h>

#include <protocol/rtp.h>
#include <protocol/acc.h>

class AAC_packetizer {
private:
    // 内部状态跟踪 
    std::unique_ptr<uint8_t[]> current_frame_;
    size_t frame_size_ = 0;
    bool has_more_packets_ = false;
    const size_t mtu_ = 1400;
    uint16_t sequence_number_ = 0;
    uint32_t ssrc_ = 0;
    uint32_t timestamp_ = 0;
    uint32_t dtime_ = 0;
public: 
    // 操作符重载：处理NAL单元并返回RTP包 
    auto set(uint32_t ssrc ,uint32_t dtime)
    {
        this->dtime_=dtime;
        this->ssrc_=ssrc;
    }

    auto operator = (
        std::pair<std::unique_ptr<uint8_t[]>, size_t> nal_unit) {
        current_frame_ = std::move(nal_unit.first); 
        frame_size_ = nal_unit.second; 
        has_more_packets_ = true;
    }
 
    // 判断是否还有更多RTP包需要读取 
    explicit operator bool() const {
        return has_more_packets_;
    }
    
    // 获取下一个RTP包
    auto operator()()->std::pair< std::unique_ptr<uint8_t[]>, size_t>  {
        const size_t packet_size = sizeof(rtp_header) + frame_size_ + 4 -7;
        auto packet = std::make_unique<uint8_t[]>(packet_size);
        rtp_header* header = reinterpret_cast<rtp_header*>(packet.get());
        header->version         = 2;
        header->padding         = 0;
        header->extension       = 0;
        header->csrc_count      = 0;
        header->marker          = 0;
        header->payload_type    = 97;
        header->sequence_number =htons( sequence_number_);
        header->ssrc =htonl(this->ssrc_);
        header->timestamp =htonl(this->timestamp_) ;
        header->marker = 1;   
        
        //AU 头
        packet[sizeof(rtp_header)]      =   0x00;
        packet[sizeof(rtp_header)+1]    =   0x10;
        packet[sizeof(rtp_header)+2]    =   ((frame_size_ - 7) & 0x1FE0) >> 5; //高8位
        packet[sizeof(rtp_header)+3]    =   ((frame_size_ - 7) & 0x1F) << 3;   //低5位

        
        //DLOG(RTP,"maker %d sequence_number %d timestamp %d  packet_size %u", static_cast<bool>(header->marker),static_cast<int>(ntohs(header->sequence_number)),ntohl(header->timestamp),packet_size);
        has_more_packets_ = false;
        std::copy_n(current_frame_.get()+7, frame_size_ - 7, packet.get() + sizeof(rtp_header) + 4);
        ++sequence_number_;
        this->timestamp_ += this->dtime_;   
        return {std::move(packet), packet_size};
    }
private:

};