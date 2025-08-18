#include<utility/log.hpp>
#include<utility/threadpool.hpp>
#include<network/rtsp_server.hpp>

int main(int argc, char const *argv[])
{   
    try
    {
        DLOG(INFO,"%s","threadpoll start");
        threadpool polls(12);

        rtsp_server srv(polls,"../data");
        srv.bind("0.0.0.0",8554,8001,8002);
        srv.listen();
        srv.start();
        DLOG(INFO,"server started at %s:%d",srv.addr,srv.port);

        polls.keep();
    }
    catch(const std::exception& e)
    {
        DLOG(EXCEP,"%s",e.what());
    }
    DLOG(INFO,"%s","main exit");
}
