#pragma once
#include <cstdarg>
#include <iostream>

#define CLOG(title,bool,...) __log__(std::cout,bool,#title,__FILE__,__LINE__,__VA_ARGS__) 
#define DLOG(title,...) __log__(std::cout,0,#title,__FILE__,__LINE__,__VA_ARGS__) 

template <typename STD_STREAM>
__inline__ void __log__(STD_STREAM& ostream, bool showFileLine, 
                   const char* title, const char* file, int line,
                   const char* format, ...) {
    // 获取当前时间 
    time_t now = time(nullptr);
    char timeStr[20] = {};
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // 处理可变参数
    char message[64] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // 输出日志
    if (showFileLine) {
        ostream << file << " " << line << " ";
    }
    ostream << timeStr << " --" << title << "-- " << message << std::endl;
}
 