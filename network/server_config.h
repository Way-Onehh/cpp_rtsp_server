#pragma  once

#include <utility/threadpool.hpp>
#include <protocol/sdp.hpp>
#include <network/udpchannel.h>

class server_config
{
public:

    virtual void init(threadpool * pools, std::map<int,sockaddr> *clients)=0;

    virtual void bind(std::string_view addr,std::initializer_list<int> list)=0;
    
    virtual void start()=0;

    virtual void add_video_sdp(sdp & sdp,std::string path)=0;

    virtual void add_audio_sdp(sdp & sdp,std::string path)=0;

    virtual std::string Transport(int port1 ,int SSRC)=0;

    virtual std::shared_ptr<channel> getchannel(std::initializer_list<int> args) = 0;
};