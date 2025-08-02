#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <log.hpp>
// eg:
//     stream_server srv;
//     srv.bind("0.0.0.0",8888);
//     srv.listen();
//     threadpool polls(12);
//     while (1)    
//     {   
//         auto ret = polls.submit(std::bind(&stream_server::accept,  &srv));
//         ret.wait();
//     }

class stream_server
{
public: 
    stream_server() :stop(false)
    {
        srv_fd =  socket(AF_INET,SOCK_STREAM,0);
        int enable = 1;
        setsockopt(srv_fd , SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
        setsockopt(srv_fd, SOL_SOCKET, SO_LINGER, &enable, sizeof(enable));
        if(srv_fd < 0) throw std::runtime_error("failed init socket");
        
        //初始化epoll
        ev.data.fd = this->srv_fd;
        ev.events = EPOLLIN | EPOLLET;
        fcntl(ev.data.fd, F_SETFL,fcntl(ev.data.fd,  F_GETFL)| O_NONBLOCK); 
        this->epoll_fd = epoll_create(1024);
        epoll_ctl(epoll_fd,EPOLL_CTL_ADD,ev.data.fd,&ev);
    }
    ~stream_server()
    {
        stop = true;
        close(this->epoll_fd);
        epoll_ctl(epoll_fd,EPOLL_CTL_DEL,srv_fd,0);
        close(this->srv_fd);
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

    //开始监听
    void listen()
    {
        int ret = ::listen(this->srv_fd,128);
        if(ret < 0) throw std::runtime_error("failed init bind");
    }
    
    // 接受一次连接
    // eg:
    //     stream_server srv;
    //     srv.bind("0.0.0.0",8888);
    //     srv.listen();
    //     threadpool polls(12);
    //     while (1)    
    //     {   
    //         auto ret = polls.submit(std::bind(&stream_server::accept,  &srv));
    //         ret.wait();
    //     }
    int accept()
    {
        if(!stop)
        {
            int nfds = epoll_wait(epoll_fd,events,1024,-1);
            if (nfds == -1) throw std::runtime_error("failed accept");
            for (size_t i = 0; i < nfds; i++)
            {
                doepoll(events[i].data.fd , events[i].events);
            }
        }
        return 0;
    }

    private:
    
    //使用epoll IO复用
    void doepoll(int fd , int event)
    {
        if(fd == this->srv_fd && event == EPOLLIN)  {handle_accept();    return;}
        if(event == EPOLLIN)                         handle_client(fd);   
    }
    
    //接受新的连接，使用epoll方法
    void handle_accept()
    {
        while (1)
        {
            ev.data.fd = ::accept(this->srv_fd,0,0);
            if (ev.data.fd == -1) {
                if (errno == EAGAIN) break; 
                continue;
            }
            ev.events = EPOLLIN | EPOLLET;
            fcntl(ev.data.fd, F_SETFL,fcntl(ev.data.fd,  F_GETFL)| O_NONBLOCK); 
            epoll_ctl(epoll_fd,EPOLL_CTL_ADD,ev.data.fd,&ev);
            DLOG(CONN,"fd = %d",ev.data.fd);
        }
    }

    //处理客户端的数据流与连接与断开
    void handle_client(int fd)
    {
        while (1)
        {
            int bytes = handle_stream(fd);
            if (bytes == -1 && errno == EAGAIN) {
                break; // 数据已读完 
            }
            else if(bytes <= 0)
            { // 客户端断开 
                close(fd);
                DLOG(DISCONN,"fd : %d",fd);
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,0);
                break; 
            }
        }
    }
    //读取报文，因为不同协议读取报文的流程不同，这里使用多态
    //返回报文大小 使用沿边触发
    virtual int handle_stream(int fd)
    {
        char buffer[1024]={0};//大于1024 多读几次
        int bytes = -1;

        bytes = read(fd, buffer, 1024);
        if (bytes > 0)
        {
            buffer[bytes] = '\0';
            DLOG(INFO,"fd : %d Size : %d\n ", fd , bytes);
            write(fd, buffer, bytes); // 回显 
        }

        return bytes;
    }

public:
    const char * addr = nullptr;
    int port = -1;
private:
    int srv_fd = -1;
    int epoll_fd  = -1;
    struct epoll_event ev,events[1024] = {0};
    bool stop = true;
};