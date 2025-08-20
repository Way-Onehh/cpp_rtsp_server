#pragma once

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <unistd.h>
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>

#include <utility/log.hpp>
#include <utility/threadpool.hpp>
#include <protocol/sdp.hpp>
#include <protocol/rtsp.hpp>
#include <network/session.hpp>
#include <network/stream_server.h>
#include <network/rtp_server.h>
#include <network/server_config.h>

#define BUFSIZE 1024

class rtsp_server : public stream_server
{
public:
    rtsp_server(threadpool &polls,server_config * server_config,std::filesystem::path workpath = std::filesystem::current_path()): stream_server(polls)
    {   
        cfg.reset(server_config);
        cfg->init(&polls);
        this->workpath = workpath;
        if(!std::filesystem::exists(workpath / "h264"))
            std::filesystem::create_directory(workpath / "h264");  
        if(!std::filesystem::exists(workpath / "aac"))
            std::filesystem::create_directory(workpath / "aac");  
        stream_server::handle_close =[this](int fd)
        {   
            auto it = sessions_by_fd.find(fd);
            if(it == sessions_by_fd.end())  return ;
            it->second->teardown();
            int id = it->second->id;
            this->sessions_by_id.erase(sessions_by_id.find(id));
            this->sessions_by_fd.erase(it);
        };
    };
    
    void bind(std::string_view addr,std::initializer_list<int> list)
    {   //绑定端口
        int port = *list.begin();
        stream_server::bind(addr,port);
        cfg->bind(addr,list);
    }
    
    void start()
    {
        //开始服务器
        stream_server::start();
        cfg->start();
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
        case Method::TEARDOWN:
            return handle_TEARDOWN(fd,req);
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
        int session_id = req.keys.count("Session")  ? std::stoi(req.keys["Session"])  : current_session_index ;
        res.keys["Session"]         =  std::to_string(session_id);
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(OPTI,"id %d %s ",session_id,"reply");
        return true;
    }

    //处理DESCRIBE方法
    bool handle_DESCRIBE(int fd, request &req)
    {
        response res;
        res.version                     = 0;
        res.code                        = 404;
        res.reason                      = "NO Found";
        res.CSeq                        =  req.CSeq;
        int session_id = req.keys.count("Session")  ? std::stoi(req.keys["Session"])  : current_session_index ;
        res.keys["Session"]         =  std::to_string(session_id);

        auto stream = request::getroot(req.url);
        sdp sdp;
        std::filesystem::path stream_path0 = workpath/"h264"/stream += ".h264";
        std::filesystem::path stream_path1 = workpath/"aac"/stream += ".aac";
       
        int flag = 0;
        if(std::filesystem::exists(stream_path0))
        {flag=1;cfg->add_video_sdp(sdp,stream_path0);}

        if(std::filesystem::exists(stream_path1))
        {flag=1;cfg->add_audio_sdp(sdp,stream_path1);}
        
        if(flag)
        {
            sdp.version                 = "0";
            sdp.origin                  ="- 0 0 IN IP4 127.0.0.1";
            sdp.sessionName             ="No Name";
            sdp.timings.emplace_back("0","0");

            res.version                 = 0;
            res.code                    = 200;
            res.reason                  = "OK";
            res.CSeq                    = req.CSeq;
            res.keys["Content-Base"]    = req.url; 
            res.keys["Accept"]          = "application/sdp";
            res.payload                 = sdp.serialize();
            res.keys["Content-Length"]  = std::to_string(res.payload.size());
        }

        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(DESC,"id %d %s ",session_id,"reply");
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
        std::string root = request::getroot(req.url);
        std::string streamid = request::getstream(req.url);
        session *session_obj;
        int  session_id;
        bool flag = 0;
        auto it = req.keys.find("Transport");
        if(it != req.keys.end()  && !root.empty() && !streamid.empty())
        {
            int port1 , port2;
            if(request::getport(it->second,port1, port2))
            {   
                auto session_it =  req.keys.find("Session");
                if(session_it == req.keys.end())
                {
                    session_obj = create_session(fd);
                    flag = 1;
                }
                else
                {

                    session_id = std::stoi(session_it->second);
                    auto it = sessions_by_id.find(session_id);
                    if(it != sessions_by_id.end())
                    {
                        session_obj = it->second.get();
                        flag = 1;
                    }
                    else
                    {
                        session_obj = create_session(fd);
                        flag = 1;
                    }
                }

                if(flag)
                {
                    sockaddr_in addr;
                    memcpy(&addr,&stream_server::clients[fd],sizeof(sockaddr_in));
                    addr.sin_port = htons(port1);
                    auto path =   workpath/"h264"/root;
                    int SSRC ;
                    
                    if(streamid == "trackID=0")
                    {
                        path =   workpath/"h264"/root += ".h264";
                        SSRC = session_id+100;
                        session_obj->setup0(pools, *static_cast<int*>( cfg->getprofile()),*reinterpret_cast<sockaddr*>(&addr),path,SSRC,90000/25);
                    }

                    if(streamid == "trackID=1")
                    {
                        path =   workpath/"aac"/root += ".aac";
                        SSRC = session_id+101;
                        session_obj->setup1(pools,*static_cast<int*>( cfg->getprofile()),*reinterpret_cast<sockaddr*>(&addr),path,SSRC,1024);
                    }

                    res.version                 = 0;
                    res.code                    = 200;
                    res.reason                  = "OK";
                    res.CSeq                    =  req.CSeq;
                    res.keys["Session"]         = std::to_string(session_id);
                    res.keys["Transport"]       = cfg->Transport(port1,SSRC);//不支持rtcp
                }
            }

        }
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(SETUP,"id %d %s ",session_id,"reply");
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

        sessions_by_id[session_id]->play();

        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        res.keys["Session"]         = std::to_string(session_id);
        res.keys["RTP-Info"]        = "url="+req.url+"/trackID=0;seq=0;rtptime=0,url="+req.url+"/trackID=1;seq=0;rtptime=0";
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());

        DLOG(PLAY,"id %d %s ",session_id,"reply");
        return true;
    }   

    bool handle_TEARDOWN(int fd, request &req)
    {
        response res;
        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        auto session_id =std::stoi( req.keys["Session"]);
        res.keys["Session"]         = session_id;
        auto && ret = res.serialize();
        write(fd,ret.data(),ret.size());
        DLOG(TEAR,"id %d %s ",session_id,"reply");
        return true;
    }

    //返回无权指针
    session * create_session(int fd)
    {
        std::shared_ptr<session> ptr( new session);
        auto temp1 =  sessions_by_id.emplace(current_session_index++,ptr);
        auto session_id  = temp1.first->first;
        auto temp2 =  sessions_by_fd.emplace(fd,ptr);
        auto session_obj = temp1.first->second.get();
        session_obj->id = session_id; 
        return session_obj;
    }
private:

public:
    std::shared_ptr<server_config> cfg;
    int current_session_index = 0;
    std::filesystem::path workpath;
    //<id,s* Session>
    std::map<int,std::shared_ptr<session>> sessions_by_id;
    //<fd,s* Session>
    std::map<int,std::shared_ptr<session>> sessions_by_fd;
};
