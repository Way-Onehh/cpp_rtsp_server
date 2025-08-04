#pragma once

#include <stream_server.hpp>
#include <rtsp.hpp>
#include <rtp.hpp>
#include <cstring>
#include <iostream>
#include <log.hpp>
#include <format.hpp>
#include <session.hpp>
#include <map>
#include <memory>

#define BUFSIZE 1024

class rtsp_server : public stream_server
{
public:
    rtsp_server(threadpool &threadpool ): stream_server(threadpool)
    {

    };

    void handle_stream(int fd,char *messages, int size) override
    {  
        try
        {
            if(isRtpPacket(messages,size))
            {   
                handle_packet(fd,messages,size);
            }else
            {
                request req(messages);
                handle_request(fd,req);
            }
        }
        catch(const std::exception& e)
        {  
            DLOG(EXCEP,"%s",e.what());
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
        case Method::SETUP:
            handle_SETUP(fd,req);
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
        auto &&root = request::getroot(req.url);
        formats.emplace(root,req.payload);
    }

    void handle_SETUP(int fd,request &req)
    {
        DLOG(SETUP,"%s","reply");
        response res;
        string ret;
        auto &&root = request::getroot(req.url);
        auto &&stream = request::getstream(req.url);
        if(formats.count(root) && formats.at(root).streams.count(stream))
        {
            sessions.emplace_back(current_session_index,formats.at(root));
            res.version = 0;
            res.code = 200;
            res.reason = "OK";
            res.CSeq =  req.CSeq;
            res.keys["Session"] = to_string(current_session_index);
            current_session_index ++;
            ret = res.serialize();
        }else
        {
            res.version = 0;
            res.code = 404;
            res.reason = "no found";
            res.CSeq =  req.CSeq;
            ret = res.serialize();
        }
        write(fd,ret.data(),ret.size());
    }

    //test
    void handle_packet(int fd,char * buf, int bytes)
    {
        DLOG(TEST,"%s","rtsp");
        write(fd,buf,bytes);
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
    std::map<std::string,Format> formats;
    std::vector<Session<>> sessions;
    int current_session_index = 0;
    std::vector<std::shared_ptr<channel>> channels;
};

