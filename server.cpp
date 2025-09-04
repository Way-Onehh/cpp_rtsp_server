#include <memory>
#include <thread>
#include <utility/log.hpp>
#include <utility/timer_threadpool.hpp>
#include <network/udp_config.h>
#include <network/tcp_config.h>
#include <network/rtsp_server.h>

int main(int argc, char const *argv[])
{   
    try
    {
        DLOG(INFO,"%s","threadpoll start");
        auto pools = std::make_shared<timer_threadpool>(std::thread::hardware_concurrency());
        auto srv = std::make_shared<rtsp_server>(*pools,"../data");
        srv->set(1200000,40000,44100,1024);
        srv->bind("0.0.0.0",{8554,8001,8002});
        srv->listen();
        srv->start();
        DLOG(INFO,"server started at %s:%d",srv->addr,srv->port);
        pools->keep();
    }  
    catch(const std::exception& e)
    {
        DLOG(EXCEP,"%s",e.what());
    }
    DLOG(INFO,"%s","main exit");
}
