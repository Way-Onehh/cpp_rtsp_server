#pragma once

#include <stdexcept>
#include <functional>
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
    dgram_server(threadpool & polls)  : polls(polls) , stop(false)
    {
        srv_fd =  socket(AF_INET,SOCK_DGRAM,0);
        int enable = 1;
        setsockopt(srv_fd , SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
        if(srv_fd < 0) throw std::runtime_error("failed init socket"); 

        //初始化epoll
        ev.data.fd = this->srv_fd;
        ev.events = EPOLLIN | EPOLLET;
        fcntl(ev.data.fd, F_SETFL,fcntl(ev.data.fd,  F_GETFL)| O_NONBLOCK); 
        this->epoll_fd = epoll_create(1024);
        epoll_ctl(epoll_fd,EPOLL_CTL_ADD,ev.data.fd,&ev);
    }
    //绑定地址
    void bind(std::string_view addr,int port) 
    { 
        struct sockaddr_in server_addr = {AF_INET,htons(port),0,0};
        inet_pton(AF_INET,addr.data(),&server_addr.sin_addr);
        int ret = ::bind(this->srv_fd,(struct sockaddr *) &server_addr,sizeof(server_addr));
        if(ret < 0) throw std::runtime_error("failed init bind");
        this->port = port;
        this->addr = addr.data();
    }

    //处理客户端
    int handle_client()
    {
        if(!stop)
        {
            std::string packet{};
            char buf[UDPMAX];
            sockaddr  addr{};
            socklen_t socklen =sizeof(addr);
            int nfds = epoll_wait(epoll_fd,events,1,-1);
            if (nfds == -1) throw std::runtime_error("failed accept");
            int bytes = recvfrom(srv_fd, buf, sizeof(buf), 0, &addr, &socklen); 
            handle_dgram(&addr,socklen,buf,bytes);
        }
        return 0;
    }

    virtual void handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n)
    {
        sendto(srv_fd,buf,n,0,addr,socklen);
    }

    void start()
    {
        auto call = [this]()
        {
            this->handle_client();
            if(!this->stop)
            polls.submit(std::bind(&dgram_server::start,this));
        };
        polls.submit(call);
    }

    int getfd()
    {
        return  this->srv_fd;
    }
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