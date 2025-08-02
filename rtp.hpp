#include <stdint.h>  
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <memory>
 
class packet {
public:
    // 构造函数
    explicit packet(const char* data, size_t length) {
        if (length < kMinHeaderSize) {
            throw std::invalid_argument("RTP packet too small");
        }
 
        // 解析固定头部 
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
        version_ = (ptr[0] >> 6) & 0x03;
        padding_ = (ptr[0] >> 5) & 0x01;
        extension_ = (ptr[0] >> 4) & 0x01;
        csrc_count_ = ptr[0] & 0x0F;
 
        marker_ = (ptr[1] >> 7) & 0x01;
        payload_type_ = ptr[1] & 0x7F;
 
        seq_num_ = (ptr[2] << 8) | ptr[3];
        timestamp_ = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
        ssrc_ = (ptr[8] << 24) | (ptr[9] << 16) | (ptr[10] << 8) | ptr[11];
 
        // 解析CSRC列表 
        size_t offset = 12;
        if (csrc_count_ > 0) {
            if (length < offset + csrc_count_ * 4) {
                throw std::invalid_argument("RTP packet too small for CSRC entries");
            }
            for (uint8_t i = 0; i < csrc_count_; ++i) {
                uint32_t csrc = (ptr[offset] << 24) | (ptr[offset+1] << 16) | 
                               (ptr[offset+2] << 8) | ptr[offset+3];
                csrc_.push_back(csrc);
                offset += 4;
            }
        }
 
        // 解析扩展头
        if (extension_) {
            if (length < offset + 4) {
                throw std::invalid_argument("RTP packet too small for extension header");
            }
            ext_profile_ = (ptr[offset] << 8) | ptr[offset+1];
            ext_length_ = (ptr[offset+2] << 8) | ptr[offset+3];
            offset += 4;
 
            size_t ext_bytes = ext_length_ * 4;
            if (length < offset + ext_bytes) {
                throw std::invalid_argument("RTP packet too small for extension data");
            }
            ext_data_.assign(ptr + offset, ptr + offset + ext_bytes);
            offset += ext_bytes;
        }
 
        // 解析载荷
        if (offset < length) {
            payload_.assign(ptr + offset, ptr + length);
        }
    }
 
    // 默认构造函数
    packet() : version_(2), padding_(0), extension_(0), csrc_count_(0),
                 marker_(0), payload_type_(0), seq_num_(0), timestamp_(0),
                 ssrc_(0), ext_profile_(0), ext_length_(0) {}
 
    // 转换为原始数据包 
    std::vector<char> toPacket() const {
        std::vector<char> packet;
        packet.reserve(kMinHeaderSize  + csrc_count_ * 4 + 
                      (extension_ ? 4 + ext_data_.size() : 0) + payload_.size());
 
        // 字节0 
        uint8_t byte0 = (version_ << 6) | (padding_ << 5) | 
                       (extension_ << 4) | (csrc_count_ & 0x0F);
        packet.push_back(byte0); 
 
        // 字节1
        uint8_t byte1 = (marker_ << 7) | (payload_type_ & 0x7F);
        packet.push_back(byte1); 
 
        // 字节2-3: 序列号
        packet.push_back(static_cast<char>((seq_num_  >> 8) & 0xFF));
        packet.push_back(static_cast<char>(seq_num_  & 0xFF));
 
        // 字节4-7: 时间戳
        packet.push_back(static_cast<char>((timestamp_  >> 24) & 0xFF));
        packet.push_back(static_cast<char>((timestamp_  >> 16) & 0xFF));
        packet.push_back(static_cast<char>((timestamp_  >> 8) & 0xFF));
        packet.push_back(static_cast<char>(timestamp_  & 0xFF));
 
        // 字节8-11: SSRC 
        packet.push_back(static_cast<char>((ssrc_  >> 24) & 0xFF));
        packet.push_back(static_cast<char>((ssrc_  >> 16) & 0xFF));
        packet.push_back(static_cast<char>((ssrc_  >> 8) & 0xFF));
        packet.push_back(static_cast<char>(ssrc_  & 0xFF));
 
        // CSRC列表
        for (uint32_t csrc : csrc_) {
            packet.push_back(static_cast<char>((csrc  >> 24) & 0xFF));
            packet.push_back(static_cast<char>((csrc  >> 16) & 0xFF));
            packet.push_back(static_cast<char>((csrc  >> 8) & 0xFF));
            packet.push_back(static_cast<char>(csrc  & 0xFF));
        }
 
        // 扩展头 
        if (extension_) {
            packet.push_back(static_cast<char>((ext_profile_  >> 8) & 0xFF));
            packet.push_back(static_cast<char>(ext_profile_  & 0xFF));
            packet.push_back(static_cast<char>((ext_length_  >> 8) & 0xFF));
            packet.push_back(static_cast<char>(ext_length_  & 0xFF));
            packet.insert(packet.end(),  ext_data_.begin(), ext_data_.end());
        }
 
        // 载荷 
        packet.insert(packet.end(),  payload_.begin(), payload_.end());
 
        return packet;
    }
 
    // 检查数据包有效性
    bool isValid() const {
        // 检查版本号 
        if (version_ != 2) {
            return false;
        }
 
        // 检查CSRC数量
        if (csrc_count_ > 15) {
            return false;
        }
 
        // 检查扩展头 
        if (extension_ && ext_data_.size() != ext_length_ * 4) {
            return false;
        }
 
        // 检查载荷类型
        if (payload_type_ >= 72 && payload_type_ <= 76) {
            // 保留的载荷类型
            return false;
        }
 
        return true;
    }
 
    // Getter和Setter方法 
    uint8_t getVersion() const { return version_; }
    void setVersion(uint8_t version) { version_ = version; }
 
    bool hasPadding() const { return padding_; }
    void setPadding(bool padding) { padding_ = padding; }
 
    bool hasExtension() const { return extension_; }
    void setExtension(bool extension) { extension_ = extension; }
 
    uint8_t getCsrcCount() const { return csrc_count_; }
    void setCsrcCount(uint8_t count) { 
        if (count > 15) throw std::invalid_argument("CSRC count must be <= 15");
        csrc_count_ = count; 
    }
 
    bool isMarker() const { return marker_; }
    void setMarker(bool marker) { marker_ = marker; }
 
    uint8_t getPayloadType() const { return payload_type_; }
    void setPayloadType(uint8_t type) { payload_type_ = type; }
 
    uint16_t getSeqNum() const { return seq_num_; }
    void setSeqNum(uint16_t seq) { seq_num_ = seq; }
 
    uint32_t getTimestamp() const { return timestamp_; }
    void setTimestamp(uint32_t ts) { timestamp_ = ts; }
 
    uint32_t getSsrc() const { return ssrc_; }
    void setSsrc(uint32_t ssrc) { ssrc_ = ssrc; }
 
    const std::vector<uint32_t>& getCsrcList() const { return csrc_; }
    void addCsrc(uint32_t csrc) { 
        if (csrc_count_ >= 15) throw std::runtime_error("CSRC list full");
        csrc_.push_back(csrc);
        csrc_count_++;
    }
 
    uint16_t getExtProfile() const { return ext_profile_; }
    void setExtProfile(uint16_t profile) { ext_profile_ = profile; }
 
    uint16_t getExtLength() const { return ext_length_; }
    void setExtLength(uint16_t length) { ext_length_ = length; }
 
    const std::vector<uint8_t>& getExtData() const { return ext_data_; }
    void setExtData(const std::vector<uint8_t>& data) { 
        ext_data_ = data;
        ext_length_ = data.size()  / 4;
    }
 
    const std::vector<uint8_t>& getPayload() const { return payload_; }
    void setPayload(const std::vector<uint8_t>& payload) { payload_ = payload; }
 
private:
    static constexpr size_t kMinHeaderSize = 12;
 
    // 固定头部字段
    uint8_t version_;       // 版本号（通常为2）
    uint8_t padding_;       // 填充标志
    uint8_t extension_;     // 扩展头标志 
    uint8_t csrc_count_;    // CSRC标识符数量（0-15）
    uint8_t marker_;        // 标记位（应用层定义）
    uint8_t payload_type_;  // 载荷类型（如96=H.264）
    uint16_t seq_num_;      // 序列号（网络字节序）
    uint32_t timestamp_;    // 时间戳（网络字节序）
    uint32_t ssrc_;         // 同步源标识符
 
    // 可选字段
    std::vector<uint32_t> csrc_;      // 贡献源列表
    uint16_t ext_profile_;            // 扩展头类型
    uint16_t ext_length_;             // 扩展头长度（以32位字为单位）
    std::vector<uint8_t> ext_data_;   // 扩展数据 
    std::vector<uint8_t> payload_;    // 载荷数据 
};