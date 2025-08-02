#pragma once

#include <stream_server.hpp>
#include <rtsp.hpp>
#include <rtp.hpp>
#include <cstring>
#include <iostream>
#include <log.hpp>
#include <format.hpp>
#include <map>

#define BUFSIZE 1024

class rtsp_server : public stream_server
{
public:
    int handle_stream(int fd) override
    {  

        char buf[BUFSIZE]={};
        int bytes = read(fd,buf,BUFSIZE);
        if(bytes > 0 )
        {
            try
            {
                if(isRtpPacket(buf,bytes))
                {   
                    packet pack(buf,bytes);
                    handle_packet(fd,pack);
                }else
                {
                    request req(buf);
                    handle_request(fd,req);
                }
            }
            catch(const std::exception& e)
            {  
                DLOG(EXCEP,"%s",e.what());
            }
        }
        return jumpdata(fd,buf,BUFSIZE);
    }
    
    //读取剩余数据不处理
    int jumpdata(int fd,char * buf , int n)
    {
        while (1)
        {
            int bytes =  read(fd,buf,n); 
            if( bytes <= 0) return bytes;
        }
    }

    void handle_request(int fd,request &req)
    {
        switch (req.method)
        {
        case Method::OPTIONS:
            handle_OPTIONS(fd,req);
            break;
        case Method::ANNOUNCE:
            handle_ANNOUNCE(fd,req);
            break;
        default:
            break;
        }
    }

    //处理OPTIONS方法
    void handle_OPTIONS(int fd,request &req)
    {
        DLOG(OPTIONS,"%s","reply");
        response res;
        res.version = 0;
        res.code = 200;
        res.reason = "OK";
        res.CSeq =  req.CSeq;
        res.keys["Public"] = "DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE";
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
    }
    
    void handle_ANNOUNCE(int fd,request &req)
    {
        DLOG(ANNOUNCE,"%s","reply");
        response res;
        res.version = 0;
        res.code = 200;
        res.reason = "OK";
        res.CSeq =  req.CSeq;
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        formats.emplace(req.url,req.payload);
    }

    //test
    void handle_packet(int fd,packet & packet)
    {
        DLOG(TEST,"%s","rtsp");
        auto packet_buf = packet.toPacket();
        write(fd,packet_buf.data(),packet_buf.size());
    }

    bool isRtpPacket(const char* data, size_t length) {
        // RTP 最小头部长度（RFC 3550）
        if (length < 12) return false;
        
        // 检查版本号（第1字节高2位应为2）
        const uint8_t version = (data[0] >> 6) & 0x03;
        if (version != 2) return false;
        
        // 检查载荷类型（第2字节低7位应为有效值）
        const uint8_t payload_type = data[1] & 0x7F;
        if (payload_type > 127) return false; // 0-127为有效范围 
        
        // 检查时间戳和SSRC是否为合理值（非零）
        const uint32_t timestamp = *reinterpret_cast<const uint32_t*>(data + 4);
        const uint32_t ssrc = *reinterpret_cast<const uint32_t*>(data + 8);
        if (timestamp == 0 || ssrc == 0) return false;
        
        return true;
    }
public:
    std::map<std::string,format> formats;
};

