#include <network/udp_config.h>

void udp_config::init(threadpool * pools) 
{   
    pools_ = pools;
    //初始化两个rtp服务器
    rtp_servers.push_back(*pools_);
    rtp_servers.push_back(*pools_);
}

void udp_config::bind(std::string_view addr,std::initializer_list<int> list)
{   
    //绑定端口
    int port1 = *(list.begin()+1);
    int port2 = *(list.begin()+2);
    rtp_servers[0].bind(addr,port1);
    rtp_servers[1].bind(addr,port2);
}

void udp_config::start()
{
    //开始服务器
    rtp_servers[0].start();
    rtp_servers[1].start();
}

void udp_config::add_video_sdp(sdp & sdp,std::string path)
{
    auto & mdp =sdp.mediaDescriptions.emplace_back();   
    mdp.media                       = "video";                  //视频流类型
    mdp.port                        = "0";                      //动态约定端口
    mdp.proto                       = "RTP/AVP";                //协议 udp
    mdp.fmt                         = "96" ;                    //类型
    mdp.attributes.emplace_back("control:trackID=0");        //流id
    mdp.attributes.emplace_back("rtpmap:96 H264/90000");     //时间基
    mdp.attributes.emplace_back(std::string("fmtp:96 packetization-mode=1;"));
    return ;
}

void udp_config::add_audio_sdp(sdp & sdp,std::string path)
{
    auto & mdp =sdp.mediaDescriptions.emplace_back();   
    mdp.media                       = "audio";                  //视频流类型
    mdp.port                        = "0";                      //动态约定端口
    mdp.proto                       = "RTP/AVP";                //协议 udp
    mdp.fmt                         = "97" ;                    //类型
    mdp.attributes.emplace_back("control:trackID=1");         //流id
    mdp.attributes.emplace_back("rtpmap:97 mpeg4-generic/44100/2;");     
    mdp.attributes.emplace_back("fmtp:97 profile-level-id=1;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;");     
    return ;
}

std::string udp_config::Transport(int port1 ,int SSRC)
{
    return  "RTP/AVP;unicast;client_port="+std::to_string(port1)+";server_port="+std::to_string(rtp_servers[0].port)+";ssrc="+std::to_string(SSRC);
}

 void * udp_config::getprofile()
 {
    return &this->rtp_servers[0].getfd();
 }