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
        file_.open(filepath, std::ios::binary);
        if (file_) {
            buffer_ = std::vector<uint8_t>(
                std::istreambuf_iterator<char>(file_),
                std::istreambuf_iterator<char>()
            );
            file_.close();
        }
        return bool(file_);
    }
 
    // 运算符重载
    std::pair<std::unique_ptr<uint8_t[]>, size_t> operator()() {
        if(*this)
        {
            uint8_t (&arr)[7]   = *reinterpret_cast<uint8_t(*)[7]>(buffer_.data() + current_off); 
            size_t frame_lenth  = FRAME_LENGTH(arr);
            auto   frame        = std::make_unique<uint8_t[]>(frame_lenth);
            std::copy_n(buffer_.data() + current_off , frame_lenth, frame.get());
            current_off         += frame_lenth;
            return  {std::move(frame),frame_lenth};
        }
        return  {};
    }
    // 
    operator bool() const {
        return  current_off < buffer_.size();
    }
 
private:
    std::vector<uint8_t> buffer_;
    size_t current_off = 0;
};