#pragma  once

#include <protocol/sdp.hpp>
#include <network/udpchannel.h>
#include <utility/threadpool.hpp>

class server_config
{
public:
 
    virtual void init(threadpool * pools, std::map<int,sockaddr> *clients)=0;

    virtual void bind(std::string_view addr,std::initializer_list<int> list)=0;
    
    virtual void start(void * ptr = nullptr)=0;

    virtual void add_video_sdp(sdp & sdp,int time_base)=0;

    virtual void add_audio_sdp(sdp & sdp,int time_base)=0;

    virtual std::string Transport(int port1 ,int port2)=0;

    virtual std::shared_ptr<channel> getchannel(std::initializer_list<int> args) = 0;
};

inline   signal<int> * ON_CLOSE_CLIENT = nullptr;