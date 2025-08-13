#pragma once
#include "sdp.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <map>
#include <filesystem>

#include <netinet/in.h>
#include <stream_server.hpp>
#include <rtsp.hpp>
#include <log.hpp>
#include <session.hpp>
#include <rtp_server.hpp>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


#define BUFSIZE 1024

class rtsp_server : public stream_server
{
public:
    rtsp_server(threadpool &polls,std::filesystem::path workpath = std::filesystem::current_path()): stream_server(polls)
    {
        //初始化两个rtp服务器
        rtp_servers.push_back(this->pools);
        rtp_servers.push_back(this->pools);
        this->workpath = workpath;
        if(!std::filesystem::exists(workpath / "h264"))
            std::filesystem::create_directory(workpath / "h264");  
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
    void handle_stream(int fd,std::string packet) override
    {  
        std::unique_ptr<request> req_ptr;
        //捕获客户端异常
        try
        {
            //防止拆包添加重新添加数据
            req_ptr .reset(new request (packet.c_str()));
            while(!handle_request(fd,*req_ptr))
            {
                read(fd,packet);
                req_ptr .reset(new request (packet.c_str()));
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
        case Method::DESCRIBE:
            return handle_DESCRIBE(fd,req);
        case Method::SETUP:
            return handle_SETUP(fd,req);
        case Method::PLAY:
            return handle_PLAY(fd,req);
        default:
            break;
        }
        return true;
    }

    //处理OPTIONS方法
    bool handle_OPTIONS(int fd,request &req)
    {
        response res;
        res.version = 0;
        res.code = 200;
        res.reason = "OK";
        res.CSeq =  req.CSeq;
        res.keys["Public"] = "DESCRIBE,SETUP,PLAY,TEARDOWN";
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(OPTIONS,"%s","reply");
        return true;
    }

    bool handle_DESCRIBE(int fd, request &req)
    {

        response res;
        res.version                     = 0;
        res.code                        = 404;
        res.reason                      = "NO Found";
        res.CSeq                        =  req.CSeq;
    
        auto stream = request::getroot(req.url);

        std::filesystem::path stream_path = workpath/"h264"/stream += ".h264";
        if(std::filesystem::exists(stream_path))
        {   
            sdp sdp;
            sdp.version                 = "0";
            sdp.origin                  ="- 0 0 IN IP4 127.0.0.1";
            sdp.sessionName             ="No Name";
            sdp.timings.emplace_back("0","0");
            this->add_video_sdp(sdp,stream_path);
            
            res.version                 = 0;
            res.code                    = 200;
            res.reason                  = "OK";
            res.CSeq                    =  req.CSeq;
            res.keys["Content-Base"]    = req.url; 
            res.keys["Accept"]          = "application/sdp";
            res.payload                 = sdp.serialize();
            res.keys["Content-Length"]  = std::to_string(res.payload.size());
        }

        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(DESCRIBE,"%s","reply");
        return true;
    }

    bool handle_SETUP(int fd, request &req)
    {   
        response res;
        res.version                     = 0;
        res.code                        = 404;
        res.reason                      = "NO Found";
        res.CSeq                        =  req.CSeq;
        
        //默认udp
        auto it = req.keys.find("Transport");
        if(it != req.keys.end())
        {
            int port1 , port2;
            if(request::getport(it->second,port1, port2))
            {   
                int  session_id = current_session_index++;
                auto ret =  sessions.emplace(session_id,session());
                if(ret.second)
                {
                    auto &session = ret.first->second;
                    sockaddr_in addr;
                    memcpy(&addr,&stream_server::clients[fd],sizeof(sockaddr_in));
                    auto stream = request::getroot(req.url);
                    auto path =   workpath/"h264"/stream += ".h264";
                    addr.sin_port = htons(port1);
                    session.setup(pools,rtp_servers[0].getfd(),*reinterpret_cast<sockaddr*>(&addr),path,session_id+100,360);
                    
                    res.version                 = 0;
                    res.code                    = 200;
                    res.reason                  = "OK";
                    res.CSeq                    =  req.CSeq;
                    res.keys["Session"]         = std::to_string(session_id);
                    res.keys["Transport"]       = std::string("RTP/AVP;unicast;client_port=24182-24183;server_port=")+std::to_string(rtp_servers[0].port);
                }
            }

        }
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(SETUP,"%s","reply");
        return true;
    }

    bool handle_PLAY(int fd, request &req)
    {
        response res;
        res.version                     = 0;
        res.code                        = 404;
        res.reason                      = "NO Found";
        res.CSeq                        =  req.CSeq;

        auto session_id =  std::stoi(req.keys["Session"]);
        sessions[session_id].play();

        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        res.keys["Session"]         = std::to_string(session_id);
        res.keys["RTP-Info"]        = "url=rtsp://localhost:8554/mystream/trackID=0;seq=0;rtptime=0";
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());

        DLOG(PLAY,"%s","reply");
        return true;
    }   

private:
    auto add_video_sdp(sdp & sdp,std::string path)->void
    {
        auto & mdp =sdp.mediaDescriptions.emplace_back();   
        mdp.media                       = "video";                  //视频流类型
        mdp.port                        = "0";                      //动态约定端口
        mdp.proto                       = "RTP/AVP";                //协议 udp
        mdp.fmt                         = "96" ;                      //类型
        mdp.attributes.emplace_back("control:trackID=0");        //流id
        mdp.attributes.emplace_back("rtpmap:96 H264/90000");     //时间基
        mdp.attributes.emplace_back(std::string("fmtp:96 packetization-mode=1;"));//+sdp::generate_fmtp_from_h264_file(path));
        //mdp.attributes.emplace_back("framerate:30");
        //packetization-mode=1 使用混合模式 fu与单包混用");
        //profile-level-id     profile 配置与 level级别      实际上sps里有
        //sprop-parameter-sets base64加密的sps pps           实际上使用rtp传输的sps pps nal数据单元
        return ;
}

public:
    int current_session_index = 0;
    std::filesystem::path workpath;
    std::vector<rtp_server> rtp_servers;
    //<id,s* Session>
    std::map<int,session> sessions;
};

