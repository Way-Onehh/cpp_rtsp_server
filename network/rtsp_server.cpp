#include <network/tcp_config.h>
#include <network/udp_config.h>
#include <network/rtsp_server.h>
#include <stream/live_stream.hpp>
#include <stream/record_stream.hpp>

bool operator<(const fd_ch& a, const fd_ch& b) {
    if (a.fd  != b.fd)  return a.fd  < b.fd; 
    return a.ch  < b.ch; 
}

rtsp_server::rtsp_server(threadpool &polls,std::filesystem::path workpath): stream_server(polls)
{   
    cfgs.emplace_back(new udp_config);
    cfgs.emplace_back(new tcp_config);
    for(auto & cfg : cfgs)
    {
        cfg->init(&polls,&clients);
    }
    this->workpath = workpath;
    auto path = this->workpath / "record";
    if(!std::filesystem::exists(path))
    {std::filesystem::create_directory(path);}  
    on_close_client_.connect(
        [&](int fd)
        {
            if(!sessions_factory.count(fd))  return ;
            auto it = sessions_factory.at(fd);
            std::string path0;
            std::string path1;
            if( it->video_stream_.lock())
            {
                path0 = it->video_stream_.lock()->path_;
                if(it->status == session::ANNOUNCING)
                {
                    stream_factory.remove(path0);
                    if(mdp_factory.count(path0)) mdp_factory.remove(path0);
                }
            }
            if( it->audio_stream_.lock())
            {
                path1 = it->audio_stream_.lock()->path_;
                if(it->status == session::ANNOUNCING)
                {
                    stream_factory.remove(path1);
                    if(mdp_factory.count(path1)) mdp_factory.remove(path1);
                }
            }
            it->teardown();
            sessions_factory.remove(fd);
            stream_server::close_client(fd);
        }
    );
    ON_CLOSE_CLIENT = &on_close_client_;
};
    
void rtsp_server::bind(std::string_view addr,std::initializer_list<int> list)
{   //绑定端口
    int port = *list.begin();
    stream_server::bind(addr,port);
    
    for(auto & cfg : cfgs)
    {
        cfg->bind(addr,list);
    }
}
    
void rtsp_server::start()
{
    //开始服务器
    stream_server::start();
    for(auto & cfg : cfgs)
    {
        cfg->start(this);
    }
}

void rtsp_server::set(uint32_t video_time_base,uint32_t video_frame_size,uint32_t audio_time_base,uint32_t audio_frame_size)
{
    video_time_base_ = video_time_base;
    video_frame_size_ = video_frame_size;
    audio_time_base_ = audio_time_base;
    audio_frame_size_ = audio_frame_size;
}

void rtsp_server::handle_stream(int fd) 
{   
    try {
        read(fd,raw_packet);
        int current_size = raw_packet.size() - index;
        while (1) {
            handle_rtsp(fd);
            handle_rtp_rtcp(fd);
            if(current_size == raw_packet.size() - index)
            {
                break;
            }else
            {
                current_size = raw_packet.size() - index;
            }
        }
        raw_packet =raw_packet.substr(index); 
        index = 0;
    } catch (std::exception &e) {
        DLOG(EXCE,"%s",e.what());
    }

}

void rtsp_server::handle_rtsp(int fd)
{
    request req;
    while (raw_packet.size()  - index >0) {
        auto consume_bytes = req.parse(raw_packet.data() + index);
        if(consume_bytes > 0)
        {
            handle_request(fd,req);
            index += consume_bytes;
        }else
        {
            break;
        }
    }
}

void rtsp_server::handle_rtp_rtcp(int fd)
{
    if(!raw_packet.empty())
    {
        auto tcp_buf = raw_packet.data() + index;
        uint8_t     channel = -1;
        uint16_t    size = 0;
        int paket_size = raw_packet.size() - index;
        
        while (paket_size >= 4 && tcp_buf[0] == 0x24)
        {
            channel = tcp_buf[1];
            size = (tcp_buf[2]<< 8 | 0x00FF & tcp_buf[3]);
            if(size <= paket_size - 4)
            {
                fd_ch fc = {fd,channel};
                if(factory_signal.count(fc))
                {
                    auto packet = std::make_shared<uint8_t[]>(size);
                    memcpy(packet.get(), tcp_buf+4, size);
                    factory_signal.at(fc)->emit(std::make_pair(std::move(packet), size));
                }
                index += (size+4);
                paket_size = raw_packet.size() - index;
                tcp_buf = raw_packet.data() + index ;
            }else
            {
                break;
            }
        }
    }
}

bool rtsp_server::handle_request(int fd,request &req)
{
    switch (req.method)
    {
    case Method::OPTIONS:
        return handle_OPTIONS(fd,req);
    case Method::DESCRIBE:
        return handle_DESCRIBE(fd,req);
    case Method::ANNOUNCE:
        return handle_ANNOUNCE(fd,req);
    case Method::SETUP:
        return handle_SETUP(fd,req);
    case Method::PLAY:
        return handle_PLAY(fd,req);
    case Method::RECORD:
        return handle_RECORD(fd,req);
    case Method::TEARDOWN:
        return handle_TEARDOWN(fd,req);
    default:
        break;
    }
    return true;
}

//处理OPTIONS方法
bool rtsp_server::handle_OPTIONS(int fd,request &req)
{
    response res;
    res.version = 0;
    res.code = 200;
    res.reason = "OK";
    res.CSeq =  req.CSeq;
    res.keys["Public"] = "DESCRIBE,SETUP,PLAY,ANNOUNCE,RECORD,TEARDOWN";
    int session_id = req.keys.count("Session")  ? 
    std::stoi(req.keys["Session"])  :
    sessions_factory.create(fd,&pools,fd,current_session_index++,session::tcp)->id;
    res.keys["Session"]         =  std::to_string(session_id);

    sessions_factory.at(fd)->keep_alive();
    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    DLOG(OPTI,"id %d %s ",session_id,"reply");
    return true;
}

//处理DESCRIBE方法
bool rtsp_server::handle_DESCRIBE(int fd, request &req)
{
    response res;
    res.version                     = 0;
    res.code                        = 404;
    res.reason                      = "NO Found";
    res.CSeq                        =  req.CSeq;
    
    int session_id = req.keys.count("Session")  ? 
    std::stoi(req.keys["Session"])  :
    sessions_factory.create(fd,&pools,fd,current_session_index++,session::tcp)->id;
    res.keys["Session"]         =  std::to_string(session_id);

    sessions_factory.at(fd)->keep_alive();
    auto stream_root = request::getroot(req.url);
    sdp sdp_;
    sdp_.version                 = "0";
    sdp_.origin                  ="- 0 0 IN IP4 127.0.0.1";
    sdp_.sessionName             ="No Name";
    sdp_.timings.emplace_back("0","0");
    std::string  stream_path0 = workpath.string()+ stream_root + ".h264";
    std::string  stream_path1 = workpath.string()+ stream_root + ".aac";
    int flag = 0;
    if(stream_root.substr(0,sizeof("/record")-1) == "/record")
    {
        if(std::filesystem::exists(stream_path0))
        {flag=1;getcofig(fd)->add_video_sdp(sdp_,video_time_base_);}
        if(std::filesystem::exists(stream_path1))
        {flag=1;getcofig(fd)->add_audio_sdp(sdp_,audio_time_base_);}
        
    }
    else
    {
        if(mdp_factory.count(stream_path0))
        {flag=1;sdp_.mediaDescriptions.push_back(*mdp_factory.at(stream_path0));}
        if(mdp_factory.count(stream_path1))
        {flag=1;sdp_.mediaDescriptions.push_back(*mdp_factory.at(stream_path1));}
    }

    if(flag)
    {
        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        res.keys["Content-Base"]    = req.url; 
        res.keys["Accept"]          = "application/sdp";
        res.payload                 = sdp_.serialize();
        res.keys["Content-Length"]  = std::to_string(res.payload.size());
    }

    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    DLOG(DESC,"id %d %s ",session_id,"reply");
    return true;
}

bool rtsp_server::handle_ANNOUNCE(int fd,request &req)
{
    response res;
    res.version                     = 0;
    res.code                        = 404;
    res.reason                      = "NO Found";
    res.CSeq                        =  req.CSeq;
    int content_length = req.keys.count("Content-Length") ?
                            std::stoi(req.keys["Content-Length"]) : 0;

    int session_id = req.keys.count("Session")  ? 
    std::stoi(req.keys["Session"])  :
    sessions_factory.create(fd,&pools,fd,current_session_index++,session::tcp)->id;
    res.keys["Session"]         =  std::to_string(session_id);
    bool flag = req.payload.size() == content_length;
    auto root = request::getroot(req.url);
    if(flag)
    {   
        sdp temp;
        temp.parse(req.payload);
        for (int i = 0 ; i < temp.mediaDescriptions.size() ;i++)
        {
            auto it =std::find(temp.mediaDescriptions[i].attributes.begin(),temp.mediaDescriptions[i].attributes.end(),std::string("control:streamid=")+std::to_string(i));
            if(it!= temp.mediaDescriptions[i].attributes.end())
            {
                *it = std::string("control:trackID=") + std::to_string(i);
            }
            std::string path;
            if(temp.mediaDescriptions[i].media == "audio")
            {
                path = workpath.c_str() + root + ".aac";
            }
            if(temp.mediaDescriptions[i].media == "video")
            {
                path = workpath.c_str() + root + ".h264";
            }
            mdp_factory.create(path, temp.mediaDescriptions[i]);
        }
    } 
        
    if(!content_length || !flag) return false;
    
    sessions_factory.at(fd)->status = session::ANNOUNCING;
    res.version                 = 0;
    res.code                    = 200;
    res.reason                  = "OK";
    res.CSeq                    = req.CSeq;
    sessions_factory.at(fd)->keep_alive();
    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    DLOG(ANNOUNCE,"id %d %s ",session_id,"reply");
    return true;
}

bool rtsp_server::handle_SETUP(int fd, request &req)
{   
    response res;
    res.version                     = 0;
    res.code                        = 404;
    res.reason                      = "NO Found";
    res.CSeq                        =  req.CSeq;

    std::string root        = request::getroot(req.url);
    std::string streamname    = request::getstream(req.url); //vlc 为""
    std::string Transport   = req.keys.count("Transport")  ? req.keys.at("Transport")  : "";
    
    int port1_or_channel_n1,port2_or_channel_n2;
    bool flag = 0;
    if(Transport !="")
    {flag = request::get_port_or_channel(Transport,port1_or_channel_n1,port2_or_channel_n2,sessions_factory.at(fd)->type_);}

    int session_id = -1; 
    std::shared_ptr<session> session_obj ;
    
    if(root != "" && flag)
    {
        session_id = req.keys.count("Session")  ? 
        std::stoi(req.keys["Session"])  :
        sessions_factory.create(fd,&pools,fd,current_session_index++,session::tcp)->id;
        session_obj = sessions_factory.at(fd);
        flag = setup(fd,root,streamname,port1_or_channel_n1,port2_or_channel_n2);
    }
    else 
    {
        flag = 0;
    }

    if(flag)
    {
        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    =  req.CSeq;
        res.keys["Session"]         = std::to_string(session_id);
        res.keys["Transport"]       = getcofig(fd)->Transport(port1_or_channel_n1,port2_or_channel_n2);
    }
    sessions_factory.at(fd)->keep_alive();
    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    DLOG(SETU,"id %d %s ",session_id,"reply");
    return true;
}

bool rtsp_server::handle_PLAY(int fd, request &req)
{
    response res;
    res.version                     = 0;
    res.code                        = 404;
    res.reason                      = "NO Found";
    res.CSeq                        =  req.CSeq;
    int session_id = -1;
    if(req.keys.count("Session"))
    {session_id =  std::stoi(req.keys["Session"]);}
    
    if(sessions_factory.count(fd)) 
    {
        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        if(session_id != -1)
        res.keys["Session"]         = std::to_string(session_id);
    }

    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    sessions_factory.at(fd)->keep_alive();
    sessions_factory.at(fd)->play();
    DLOG(PLAY,"id %d %s ",session_id,"reply");
    return true;
}   

bool rtsp_server::handle_RECORD(int fd, request &req)
{
    response res;
    res.version                     = 0;
    res.code                        = 404;
    res.reason                      = "NO Found";
    res.CSeq                        =  req.CSeq;
    int session_id = -1;
    if(req.keys.count("Session"))
    {session_id =  std::stoi(req.keys["Session"]);}
    
    if(sessions_factory.count(fd)) 
    {
        res.version                 = 0;
        res.code                    = 200;
        res.reason                  = "OK";
        res.CSeq                    = req.CSeq;
        if(session_id != -1)
        res.keys["Session"]         = std::to_string(session_id);
    }

    auto && ret = res.serialize();
    write(fd,ret.data(),ret.size());
    sessions_factory.at(fd)->keep_alive();
    DLOG(RECORD,"id %d %s ",session_id,"reply");
    return true;
}

bool rtsp_server::handle_TEARDOWN(int fd, request &req)
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

std::shared_ptr<server_config> rtsp_server::getcofig(int fd)
{
    switch (sessions_factory.at(fd)->type_) 
    {
        case  session::type::tcp:
            return cfgs[1];
        break;
        case  session::type::udp:
            return cfgs[0];
        break;           
    }
}

bool rtsp_server::setup(int fd,std::string root,std::string streamname,int port1_or_channel_n1,int port2_or_channel_n2)
{
    auto session_obj= sessions_factory.at(fd);
    auto finish_slot =[this](std::string path)
        {
            stream_factory.remove(path);
        };
    
    switch (int(session_obj->status)) {
        case session::INIT:
        case session::SETUP:
            return setup_pull_stream(session_obj,finish_slot,fd,root,streamname,port1_or_channel_n1,port2_or_channel_n2);
            break;
        case session::ANNOUNCING:
            return setup_push_stream(session_obj,finish_slot,fd,root,streamname,port1_or_channel_n1,port2_or_channel_n2);
        break;
    }
    return false;
}

bool rtsp_server::setup_pull_stream(std::shared_ptr<session> session_obj ,const std::function<void(std::string)> & finish_slot, int fd,std::string root,std::string_view streamname,int port1_or_channel_n1,int port2_or_channel_n2)
{
    if(streamname == "/trackID=0")
    {
        auto path =   workpath.string() + root + ".h264";
        auto SSRC = session_obj->id+100;
        std::shared_ptr<stream> stream ;
        if(root.substr(0,sizeof("/record")-1) == "/record")
        {            
        stream = stream_factory.count(path) ? 
        stream_factory.at(path): 
        stream_factory.create<record_stream<record_video>>(path,&pools,path,SSRC,video_time_base_,video_frame_size_,finish_slot);
        } else if(stream_factory.count(path))
        {
            stream = stream_factory.at(path);
        }else
        {
            return false;
        }
        session_obj->setup0(stream, getcofig(fd)->getchannel({fd,port1_or_channel_n1}),getcofig(fd)->getchannel({fd,port2_or_channel_n2}));
    }

    if(streamname == "/trackID=1" )
    {
        auto path =   workpath.string() + root + ".aac";
        auto SSRC = session_obj->id+100;
        std::shared_ptr<stream> stream;
        if(root.substr(0,sizeof("/record")-1) == "/record")
        {
            stream = stream_factory.count(path) ?
            stream_factory.at(path) : 
            stream_factory.create<record_stream<record_audio>>(path,&pools,path,SSRC,audio_time_base_,audio_frame_size_,finish_slot);
        }
        else if(stream_factory.count(path))
        {
            stream = stream_factory.at(path);
        }else
        {
            return false;
        }
        session_obj->setup1(stream, getcofig(fd)->getchannel({fd,port1_or_channel_n1}),getcofig(fd)->getchannel({fd,port2_or_channel_n2}));
    }
    return true;
}

bool rtsp_server::setup_push_stream(
    std::shared_ptr<session> session_obj ,const std::function<void(std::string)> & finish_slot,
    int fd,std::string root,std::string_view streamname,int port1_or_channel_n1,int port2_or_channel_n2)
{
    if(streamname == "/streamid=0")
    {
        auto path =  workpath.c_str() + root + ".h264";
        std::shared_ptr<stream> stream = stream_factory.count(path) ?
        stream_factory.at(root) : 
        stream_factory.create<live_stream>(path,nullptr,path,0,0,0,finish_slot);
        session_obj->setup0(stream, getcofig(fd)->getchannel({fd,port1_or_channel_n1}),getcofig(fd)->getchannel({fd,port2_or_channel_n2}));
        session_obj->announce0();
    }

    if(streamname == "/streamid=1")
    {
        auto path =  workpath.c_str() + root + ".aac";
        std::shared_ptr<stream> stream = stream_factory.count(path) ?
        stream_factory.at(root) : 
        stream_factory.create<live_stream>(path,nullptr,path,0,0,0,finish_slot);
        session_obj->setup1(stream, getcofig(fd)->getchannel({fd,port1_or_channel_n1}),getcofig(fd)->getchannel({fd,port2_or_channel_n2}));
        session_obj->announce1();
    }  
    return true;
}   
