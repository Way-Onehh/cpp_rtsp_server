#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <map>


#include <stream_server.hpp>
#include <rtsp.hpp>
#include <log.hpp>
#include <format.hpp>
#include <session.hpp>
#include <rtp_server.hpp>
#include <sys/socket.h>
#include <unistd.h>


#define BUFSIZE 1024

class rtsp_server : public stream_server
{
public:
    rtsp_server(threadpool &polls): stream_server(polls)
    {
        //初始化两个rtp服务器
        rtp_servers.push_back(this->polls);
        rtp_servers.push_back(this->polls);
    };

    void bind(std::string_view addr, int port , int port1,int port2)
    {   //绑定端口
        stream_server::bind(addr,port);
        rtp_servers[0].bind(addr,port1);
        rtp_servers[1].bind(addr,port2);
    }
    
    void start()
    {
        //开始服务器
        stream_server::start();
        rtp_servers[0].start();
        rtp_servers[1].start();
    }
private:
    void handle_stream(int fd,string packet) override
    {  
        std::unique_ptr<request> req_ptr;
        //捕获客户端异常
        try
        {
            if(isRtpPacket(packet.data(),packet.size()))
            {   
                handle_packet(fd,packet.data(),packet.size());
            }
            else
            {
                //防止拆包添加重新添加数据
                req_ptr .reset(new request (packet.c_str()));
                while(!handle_request(fd,*req_ptr))
                {
                    read(fd,packet);
                    req_ptr .reset(new request (packet.c_str()));
                }
            }
        }
        catch(const std::exception& e)
        {  
            DLOG(EXCEP,"%s",e.what());
        }
    }
    bool handle_request(int fd,request &req)
    {
        switch (req.method)
        {
        case Method::OPTIONS:
            return handle_OPTIONS(fd,req);
        case Method::ANNOUNCE:
            return handle_ANNOUNCE(fd,req);
        case Method::SETUP:
            return handle_SETUP(fd,req);
        case Method::RECORD:
            return handle_RECORD(fd,req);
        default:
            break;
        }
        return true;
    }

    //处理OPTIONS方法
    bool handle_OPTIONS(int fd,request &req)
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
        return true;
    }
    
    bool handle_ANNOUNCE(int fd, request &req)
    {
        if(req.payload.size() < 10) return false;
        DLOG(ANNOUNCE,"%s","reply");
        response res;
        res.version = 0;
        res.code = 200;
        res.reason = "OK";
        res.CSeq =  req.CSeq;
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        auto &&root = request::getroot(req.url);
        formats.emplace(root,new Format(req.payload));
        return true;
    }

    bool handle_SETUP(int fd, request &req)
    {
        DLOG(SETUP,"%s","reply");
        response res;
        string ret;
        auto &&root = request::getroot(req.url);
        auto &&stream = request::getstream(req.url);
        int session_id = -1;
        const auto & transport = req.keys["Transport"];
        int port1,port2;

        res.version = 0;
        res.code = 404;
        res.reason = "no found";
        res.CSeq =  req.CSeq;
        ret = res.serialize();
        
        if(formats.count(root) && formats.at(root)->streams_by_name.count(stream) && req.keys.count("Transport"))
        {   
            bool is_true = 0;
            int index = 1;

            if(!req.keys.count("Session"))
            {    
                sessions.emplace(current_session_index,new Session<>(*formats.at(root)));
                index = 0;
                session_id = current_session_index;
            }else
            {
                session_id =  std::stoi(req.keys["Session"]);
            }
            

            if(transport.find("RTP/AVP/UDP") != string::npos)
            {

                auto ret = getport(transport,port1,port2);
                if(ret)
                {
                  
                    socklen_t len = sizeof(sockaddr);
                    auto client_addr = clients.find(fd)->second;
                    struct sockaddr_in reg_addr = {AF_INET,htons(port1),((struct sockaddr_in *) &client_addr)->sin_addr,0};
                    auto session = sessions.find(session_id)->second;
                    auto slot =[session] (char *buf ,size_t n)
                    {
                        session->handle_RECORD(buf, n);
                    };
                
                    rtp_servers[index].reg_sockaddr((struct sockaddr*) &reg_addr,len,slot);
                    is_true = 1;
                }
            }

            if(is_true)
            {
                res.version = 0;
                res.code = 200;
                res.reason = "OK";
                res.CSeq =  req.CSeq;
                res.keys["Session"] = to_string(current_session_index);
                char buf[64]={};
                snprintf(buf,64,"RTP/AVP;unicast;client_port=%d;server_port=%d;rtcp-mux",port1,rtp_servers[0].port);
                res.keys["Transport"] = buf;
                current_session_index ++;
                ret = res.serialize();
            }
        }
        write(fd,ret.data(),ret.size());
        return true;
    }
    bool handle_RECORD(int fd,request req)
    {
        response res;
        res.version = 0;
        res.code = 200;
        res.reason = "OK";
        res.CSeq =  req.CSeq;
        res.keys["Session"] = req.keys["Session"];
        auto ret = res.serialize();
        write(fd,ret.data(),ret.size());
        return true;
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

    bool getport(const string &Transport,int &p1 ,int &p2)
    {
        std::regex regex(R"(client_port=(\d+)-(\d+))");
        std::smatch smatch;
        bool ret = std::regex_search(Transport,smatch,regex);
        p1 = std::stoi(smatch[1]);
        p2 = std::stoi(smatch[2]);
        return ret;
    }
public:
    int current_session_index = 0;
    std::vector<rtp_server> rtp_servers;
    //<root,>
    std::map<std::string,std::shared_ptr<Format>> formats;
    //<id,>
    std::map<int,std::shared_ptr<Session<>> > sessions;

};

