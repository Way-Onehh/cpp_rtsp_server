#pragma  once
#include <memory>
#include <string>
#include <utility/timer_threadpool.hpp>
#include <protocol/sdp.hpp>
#include <network/rtp_server.h>
#include <network/server_config.h>
#include <network/udpchannel.h>

class udp_config : public server_config
{
    threadpool*  pools_; 
    std::map<int,sockaddr> *clients_;
public:
    std::vector<std::shared_ptr<rtp_server>> rtp_servers;
    void init(threadpool * pools,std::map<int,sockaddr> *clients) ;

    void bind(std::string_view addr,std::initializer_list<int> list);

    void start(void * ptr = nullptr);

    void add_video_sdp(sdp & sdp ,int time_base);

    void add_audio_sdp(sdp & sdp ,int time_base);

    std::string Transport(int port1 ,int port2);

    std::shared_ptr<channel> getchannel(std::initializer_list<int> args);
};