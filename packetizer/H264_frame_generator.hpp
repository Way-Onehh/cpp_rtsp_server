// #pragma once

// #include "utility/log.hpp"
// #include <memory>
// #include <utility>
// #include <vector>
// #include <algorithm>
// #include <fstream>

// class H264_frame_generator {
// public:
//     // 构造函数，接收文件路径 
//     bool open(const std::string& filepath) {
//         std::ifstream file_;
//         file_.open(filepath, std::ios::binary);
//         if (file_) {
//             buffer_ = std::vector<uint8_t>(
//                 std::istreambuf_iterator<char>(file_),
//                 std::istreambuf_iterator<char>()
//             );
//             current_nal_info_ = find_next_nal_start(0);
//             file_.close();
//         }
//         return bool(file_);
//     }
 
//     // 运算符重载，返回下一个NAL单元
//     std::pair<std::shared_ptr<uint8_t[]>, size_t> operator()() {
//         if (!*this) return {nullptr, 0};
    
//         auto [start_pos, startcode_len] = current_nal_info_;
//         current_nal_info_ = find_next_nal_start(start_pos + startcode_len);
        
//         const size_t nal_size = (current_nal_info_.first == npos) 
//             ? buffer_.size() - start_pos - startcode_len  // 最后一个NAL
//             : current_nal_info_.first - start_pos - startcode_len; // 中间NAL 
//         DLOG(nal_size,"%d",nal_size);
//         auto data = std::make_shared<uint8_t[]>(nal_size);
//         std::copy_n(buffer_.data() + start_pos + startcode_len, nal_size, data.get()); 
//         return {std::move(data), nal_size};
//     }
//     // 检查是否还有NAL单元可读
//     operator bool() const {
//         return current_nal_info_.first != npos && current_nal_info_.first < buffer_.size();
//     }
 
// private:
//     static constexpr size_t npos = static_cast<size_t>(-1);
//     std::vector<uint8_t> buffer_;
//     size_t file_size_ = 0;
//     std::pair<size_t, size_t> current_nal_info_{npos, 0};
 
//     // 查找下一个NAL单元起始位置 (0x00000001 或 0x000001)
//     std::pair<size_t, size_t> find_next_nal_start(size_t start) const {
//         for (size_t i = start; i + 2 < buffer_.size(); ++i) {
//             if (buffer_[i] == 0 && buffer_[i+1] == 0 && buffer_[i+2] == 1) {
//                 bool is_long_startcode = (i > 0 && buffer_[i-1] == 0);
//                 return {is_long_startcode ? (i-1) : i, 
//                     is_long_startcode ? 4 : 3}; // 返回位置和起始码长度 
//             }
//         }
//         return {npos, 0};
//     }
// };

#pragma once
 
#include <utility/log.hpp>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <fstream>
 
class H264_frame_generator {
public:
    bool open(const std::string& filepath) {
        std::ifstream file_(filepath, std::ios::binary | std::ios::ate);
        if (!file_) return false;
        
        size_t file_size = file_.tellg();
        if (file_size == 0 || file_size > 100 * 1024 * 1024) { // 限制最大100MB
            file_.close();
            return false;
        }
        
        file_.seekg(0);
        buffer_.resize(file_size);
        if (!file_.read(reinterpret_cast<char*>(buffer_.data()), file_size)) {
            buffer_.clear();
            file_.close();
            return false;
        }
        
        current_nal_info_ = find_next_nal_start(0);
        file_.close();
        return true;
    }
 
    std::pair<std::shared_ptr<uint8_t[]>, size_t> operator()() {
        if (!*this) return {nullptr, 0};
 
        auto [start_pos, startcode_len] = current_nal_info_;
        if (start_pos >= buffer_.size()) return {nullptr, 0};
 
        current_nal_info_ = find_next_nal_start(start_pos + startcode_len);
 
        size_t nal_size = (current_nal_info_.first == npos)
            ? buffer_.size() - start_pos - startcode_len
            : current_nal_info_.first - start_pos - startcode_len;
 
        // 安全检查：确保NAL单元大小合理 
        if (nal_size == 0 || nal_size > 10 * 1024 * 1024) { // 限制最大10MB
            return {nullptr, 0};
        }
 
        // 检查数据范围是否有效
        if (start_pos + startcode_len + nal_size > buffer_.size()) {
            return {nullptr, 0};
        }
 
        auto data = std::shared_ptr<uint8_t[]>(new uint8_t[nal_size]);
        std::copy_n(buffer_.data() + start_pos + startcode_len, nal_size, data.get()); 
        return {std::move(data), nal_size};
    }
 
    explicit operator bool() const {
        return current_nal_info_.first != npos && 
               current_nal_info_.first < buffer_.size() &&
               !buffer_.empty();
    }
 
private:
    static constexpr size_t npos = static_cast<size_t>(-1);
    std::vector<uint8_t> buffer_;
    std::pair<size_t, size_t> current_nal_info_{npos, 0};
 
    std::pair<size_t, size_t> find_next_nal_start(size_t start) const {
        if (start + 2 >= buffer_.size()) return {npos, 0};
 
        for (size_t i = start; i + 2 < buffer_.size(); ++i) {
            if (buffer_[i] == 0 && buffer_[i+1] == 0 && buffer_[i+2] == 1) {
                bool is_long_startcode = (i > 0 && buffer_[i-1] == 0);
                return {is_long_startcode ? (i-1) : i,
                       is_long_startcode ? 4 : 3};
            }
        }
        return {npos, 0};
    }
};