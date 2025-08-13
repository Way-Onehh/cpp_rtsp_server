#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <fstream>

class H264_frame_generator {
public:
    // 构造函数，接收文件路径 
    bool open(const std::string& filepath) {
        file_.open(filepath, std::ios::binary);
        if (file_) {
            buffer_ = std::vector<uint8_t>(
                std::istreambuf_iterator<char>(file_),
                std::istreambuf_iterator<char>()
            );
            current_nal_info_ = find_next_nal_start(0);
            file_.close();
        }
        return bool(file_);
    }
 
    // 运算符重载，返回下一个NAL单元
    std::pair<std::unique_ptr<uint8_t[]>, size_t> operator()() {
        if (!*this) return {nullptr, 0};
    
        auto [start_pos, startcode_len] = current_nal_info_;
        current_nal_info_ = find_next_nal_start(start_pos + startcode_len);
        
        const size_t nal_size = (current_nal_info_.first == npos) 
            ? buffer_.size() - start_pos - startcode_len  // 最后一个NAL
            : current_nal_info_.first - start_pos - startcode_len; // 中间NAL 
        
        auto data = std::make_unique<uint8_t[]>(nal_size);
        std::copy_n(buffer_.data() + start_pos + startcode_len, nal_size, data.get()); 
        return {std::move(data), nal_size};
    }
    // 检查是否还有NAL单元可读
    explicit operator bool() const {
        return current_nal_info_.first != npos && current_nal_info_.first < buffer_.size();
    }
 
private:
    static constexpr size_t npos = static_cast<size_t>(-1);
    std::ifstream file_;
    std::vector<uint8_t> buffer_;
    size_t file_size_ = 0;
     std::pair<size_t, size_t> current_nal_info_{npos, 0};
 
    // 查找下一个NAL单元起始位置 (0x00000001 或 0x000001)
    std::pair<size_t, size_t> find_next_nal_start(size_t start) const {
        for (size_t i = start; i + 2 < buffer_.size(); ++i) {
            if (buffer_[i] == 0 && buffer_[i+1] == 0 && buffer_[i+2] == 1) {
                bool is_long_startcode = (i > 0 && buffer_[i-1] == 0);
                return {is_long_startcode ? (i-1) : i, 
                    is_long_startcode ? 4 : 3}; // 返回位置和起始码长度 
            }
        }
        return {npos, 0};
    }
};