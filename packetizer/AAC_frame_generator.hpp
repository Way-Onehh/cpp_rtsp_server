#pragma once

#include <cstddef>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>

#include <protocol/rtp.h>
#include <protocol/acc.h>

class AAC_frame_generator {
public:
    // 构造函数，接收文件路径 
    bool open(const std::string& filepath) {
        std::ifstream file_;
        file_.open(filepath, std::ios::binary | std::ios::ate);
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
        file_.close();
        return true;
    }
 
    // 运算符重载
    std::pair<std::shared_ptr<uint8_t[]>, size_t> operator()() {
        if(!*this) return {};

        uint8_t (&arr)[7]   = *reinterpret_cast<uint8_t(*)[7]>(buffer_.data() + current_off); 
        size_t frame_lenth  = FRAME_LENGTH(arr);
        auto   frame        = std::make_shared<uint8_t[]>(frame_lenth);
        std::copy_n(buffer_.data() + current_off , frame_lenth, frame.get());
        current_off         += frame_lenth;
        return  {std::move(frame),frame_lenth};
    }
    // 
    operator bool() const {
        return  current_off < buffer_.size();
    }
 
private:
    std::vector<uint8_t> buffer_;
    size_t current_off = 0;
};