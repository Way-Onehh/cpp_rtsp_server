#pragma once


#include <string_view>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

#include <utility/log.hpp>
#include <utility/threadpool.hpp>

#define UDPMAX 1472
//无需 listen accept 产生新的fd
//sockfd  bind 地址 从sockfd读取 报文与地址 直接发送给指定地址
//eg server  或者 client  关闭不会发送报文是无感的
class dgram_server
{
public:
    dgram_server(threadpool & polls);
    //绑定地址
    void bind(std::string_view addr,int port) ;

    //处理客户端
    int handle_client();

    virtual void handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n);

    void start();

    int&  getfd();
public:
    const char * addr = nullptr;
    int port = -1;
private:
    threadpool & polls;
    bool stop = true;
    int epoll_fd  = -1;
    struct epoll_event ev,events[1];
protected:  
    int srv_fd = -1;
};