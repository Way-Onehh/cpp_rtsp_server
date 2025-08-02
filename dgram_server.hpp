#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <log.hpp>
//无需 listen accept 产生新的fd
//sockfd  bind 地址 从sockfd读取 报文与地址 直接发送给指定地址
//eg server  或者 client  关闭不会发送报文是无感的
class dgram_server
{
public:
        dgram_server()  : stop(false)
    {
        srv_fd =  socket(AF_INET,SOCK_DGRAM,0);
        int enable = 1;
        setsockopt(srv_fd , SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
        if(srv_fd < 0) throw std::runtime_error("failed init socket");
        fcntl(srv_fd, F_SETFL,fcntl(srv_fd,  F_GETFL)| O_NONBLOCK); 
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
    virtual int handle_client()
    {
        if(!stop)
        {
            char buf[1024];
            sockaddr  addr{};
            socklen_t socklen =sizeof(addr);
            int bytes = recvfrom(srv_fd, buf, sizeof(buf), 0, &addr, &socklen);  
            sendto(srv_fd,buf,bytes,0,&addr,socklen);
        }
        return 0;
    }
public:
    const char * addr = nullptr;
    int port = -1;
private:
    int srv_fd = -1;
    bool stop = true;
};