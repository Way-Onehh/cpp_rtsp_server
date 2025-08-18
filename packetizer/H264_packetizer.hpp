#pragma once

#include <memory>
#include <cstdint>
#include <algorithm>
#include <netinet/in.h>

#include <protocol/rtp.h>
#include <protocol/h264.h>

#define UDPMAX 1472


class H264_packetizer {
private:
    // 内部状态跟踪 
    std::unique_ptr<uint8_t[]> current_nal_;
    size_t nal_size_ = 0;
    size_t offset_ = 0;
    uint16_t sequence_number_ = 0;
    bool has_more_packets_ = false;
    const size_t mtu_ = 1400;
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

    //填充数据帧
    auto operator = (
        std::pair<std::unique_ptr<uint8_t[]>, size_t> nal_unit) {
        current_nal_ = std::move(nal_unit.first); 
        nal_size_ = nal_unit.second; 
        offset_ = 0;
        has_more_packets_ = true;
    }
 
    // 判断是否还有更多RTP包需要读取 
    explicit operator bool() const {
        return has_more_packets_;
    }
    
    // 获取下一个RTP包
    auto operator()() -> std::pair<std::unique_ptr<uint8_t[]>, size_t> {
        if(*this)
        {
            if (nal_size_ <= mtu_ - sizeof(rtp_header) - sizeof(nal_header)) {
                // 单包模式
                return create_single_packet();
            } else {
                // 分片模式 
                return create_fragmented_packet();
            }
        }
        return {};
    }
private:
    // 创建单个NAL单元的RTP包
    auto create_single_packet() ->std::pair<std::unique_ptr<uint8_t[]>, size_t> {
        const size_t packet_size = sizeof(rtp_header) + nal_size_;
        auto packet = std::make_unique<uint8_t[]>(packet_size);
        
        // 填充RTP头部 
        rtp_header* header = reinterpret_cast<rtp_header*>(packet.get());
        header->version = 2;
        header->padding = 0;
        header->extension = 0;
        header->csrc_count = 0;
        header->marker = 0;
        header->payload_type = 96;
        header->sequence_number =htons( sequence_number_);
        header->ssrc =htonl(this->ssrc_);
        header->timestamp = 0;
        // 复制NAL数据
        std::copy_n(current_nal_.get(), nal_size_, packet.get()  + sizeof(rtp_header));
        uint8_t naluType = current_nal_.get()[0];
       
        //sps pps sei 不是视频帧 
        if ((naluType & 0x1F) != 6 &&  (naluType & 0x1F) != 7 && (naluType & 0x1F) != 8 )
        {
            header->timestamp =htonl(this->timestamp_) ;
            header->marker = 1;   
            this->timestamp_ += this->dtime_;   
        } 
        //DLOG(RTP,"maker %d sequence_number %d timestamp %d  packet_size %u", static_cast<bool>(header->marker),static_cast<int>(ntohs(header->sequence_number)),ntohl(header->timestamp),packet_size);
        has_more_packets_ = false;
        ++sequence_number_;
        return {std::move(packet), packet_size};
    }
 
    // 创建分片NAL单元的RTP包 
    auto create_fragmented_packet() ->std::pair<std::unique_ptr<uint8_t[]>, size_t> {
        const size_t max_payload = mtu_; // - sizeof(rtp_header) - sizeof(fu_indicator) - sizeof(fu_header)
        const size_t payload_size = std::min(max_payload, nal_size_ - offset_);
        const size_t packet_size = sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header) + payload_size;
        
        auto packet = std::make_unique<uint8_t[]>(packet_size);
        
        // 填充RTP头部 
        rtp_header* header = reinterpret_cast<rtp_header*>(packet.get()); 
        header->version = 2;
        header->padding = 0;
        header->extension = 0;
        header->csrc_count = 0;
        header->marker = (offset_ + payload_size >= nal_size_) ? 1 : 0; // 最后一个分片标记结束 
        header->payload_type = 96;
        header->sequence_number =htons( sequence_number_);
        header->timestamp =htonl(this->timestamp_) ;
        header->ssrc =htonl(this->ssrc_);
        // 获取原始NAL头
        nal_header* original_header = reinterpret_cast<nal_header*>(current_nal_.get());
 
        // 填充FU indicator 
        fu_indicator* indicator = reinterpret_cast<fu_indicator*>(packet.get()  + sizeof(rtp_header));
        indicator->forbidden_bit = original_header->forbidden_bit;
        indicator->nal_ref_idc = original_header->nal_ref_idc;
        indicator->type = 28; // FU-A类型
 
        // 填充FU header 
        fu_header* fu_hdr = reinterpret_cast<fu_header*>(packet.get()  + sizeof(rtp_header) + sizeof(fu_indicator));
        fu_hdr->start_bit = (offset_ == 0) ? 1 : 0;
        fu_hdr->end_bit = (offset_ + payload_size >= nal_size_) ? 1 : 0;
        fu_hdr->reserved_bit = 0;
        fu_hdr->nal_type = original_header->nal_unit_type;
 
        // 复制NAL数据
        std::copy_n(current_nal_.get() + offset_ + 1 ,
                   payload_size,
                   packet.get()  + sizeof(rtp_header) + sizeof(fu_indicator) + sizeof(fu_header));
        //DLOG(RTP,"maker %d sequence_number %d timestamp %d  packet_size %u \n start_bit %d  end_bit %d", static_cast<bool>(header->marker),static_cast<int>(ntohs(header->sequence_number)),ntohl(header->timestamp),packet_size,static_cast<bool>(fu_hdr->start_bit),static_cast<bool>(fu_hdr->end_bit));
        offset_ += payload_size;
        ++sequence_number_;
        has_more_packets_ = (offset_ < nal_size_);
        if(!has_more_packets_) this->timestamp_ += this->dtime_;
        return {std::move(packet), packet_size};
    }
};