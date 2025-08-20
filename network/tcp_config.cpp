#include <network/tcp_config.h>
#include <network/tcpchannel.h>


void tcp_config::init(threadpool * pools, std::map<int,sockaddr> *clients)
{
    this->pools_ = pools;
}

void tcp_config::bind(std::string_view addr,std::initializer_list<int> list)
{

}

void tcp_config::start()
{

}

void tcp_config::add_video_sdp(sdp & sdp,std::string path)
{
    auto & mdp =sdp.mediaDescriptions.emplace_back();   
    mdp.media                       = "video";                  //视频流类型
    mdp.port                        = "0";                      //动态约定端口
    mdp.proto                       = "RTP/AVP/TCP";                //协议 udp
    mdp.fmt                         = "96" ;                    //类型
    mdp.attributes.emplace_back("control:trackID=0");        //流id
    mdp.attributes.emplace_back("rtpmap:96 H264/90000");     //时间基
    mdp.attributes.emplace_back(std::string("fmtp:96 packetization-mode=1;"));
    return ;
}

void tcp_config::add_audio_sdp(sdp & sdp,std::string path)
{
    auto & mdp =sdp.mediaDescriptions.emplace_back();   
    mdp.media                       = "audio";                  //视频流类型
    mdp.port                        = "0";                      //动态约定端口
    mdp.proto                       = "RTP/AVP/TCP";                //协议 udp
    mdp.fmt                         = "97" ;                    //类型
    mdp.attributes.emplace_back("control:trackID=1");         //流id
    mdp.attributes.emplace_back("rtpmap:97 mpeg4-generic/44100/2;");     
    mdp.attributes.emplace_back("fmtp:97 profile-level-id=1;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;");     
    return ;
}

std::string tcp_config::Transport(int port1 ,int SSRC)
{
      return  "RTP/AVP/TCP;unicast;interleaved="+std::to_string(port1)+";ssrc="+std::to_string(SSRC);
}

std::shared_ptr<channel> tcp_config::getchannel(std::initializer_list<int> args)
{
    auto it = args.begin();
    int fd          = *it++;
    int ch_number   = *it;

    auto ch = std::make_shared<tcpchannel>();
    ch->set(fd,&ch_number);
    return ch;
}