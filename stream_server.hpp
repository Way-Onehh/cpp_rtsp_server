#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <log.hpp>
#include <threadpool.hpp>
#include <map>
#include <memory.h>
class stream_server
{
public: 
    stream_server(threadpool & polls) :pools(polls),stop(false)
    {
        //初始化服务器fd
        srv_fd      =   socket(AF_INET,SOCK_STREAM,0);
        int enable  =   1;
        setsockopt(srv_fd , SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
        setsockopt(srv_fd, SOL_SOCKET, SO_LINGER, &enable, sizeof(enable));
        if(srv_fd < 0) throw std::runtime_error("failed init socket");
        
        //初始化epoll
        ev.data.fd  =   this->srv_fd;
        ev.events   =   EPOLLIN | EPOLLET;
        fcntl(ev.data.fd, F_SETFL,fcntl(ev.data.fd,  F_GETFL)| O_NONBLOCK); 
        this->epoll_fd = epoll_create(1024);
        epoll_ctl(epoll_fd,EPOLL_CTL_ADD,ev.data.fd,&ev);
    }

    ~stream_server()
    {
        stop        =   true;
        close(this->epoll_fd);
        epoll_ctl(epoll_fd,EPOLL_CTL_DEL,srv_fd,0);
        close(this->srv_fd);
    }

    //绑定地址
    void bind(std::string_view addr,int port) 
    { 
        struct sockaddr_in server_addr  =   {AF_INET,htons(port),0,0};
        inet_pton(AF_INET,addr.data(),&server_addr.sin_addr);
        int ret                         =   ::bind(this->srv_fd,(struct sockaddr *) &server_addr,sizeof(server_addr));
        if(ret < 0) throw std::runtime_error("failed init bind");
        this->port = port;
        this->addr = addr.data();
    }

    //开始监听
    void listen()
    {
        int ret = ::listen(this->srv_fd,128);
        if(ret < 0) throw std::runtime_error("failed init listen");
    }
    
    //
    int accept_epoll()
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

    void start()
    {
        auto call = [this]()
        {
            this->accept_epoll();
            if(!this->stop)
            pools.submit(std::bind(&stream_server::start,this));
        };
        pools.submit(call);
    }

    //当发生拆包时，再次调用这个函数追加剩余数据
    bool read(int fd,std::string &packet)
    {
        char buf[1024]{};
        bool has_data = 0;
        int times = 0;
        while (true) {
            memset(buf,0,1024);
            int bytes = ::read(fd, buf, sizeof(buf)); // 动态缓冲区大小
            if (bytes == -1 &&(errno == EAGAIN || errno == EWOULDBLOCK)) break; 
            else if (bytes <= 0) { 
                close(fd);
                DLOG(DISCONN, "fd: %d", fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                clients.erase(fd);
                break;
            } 
            else {
                packet.append(buf, bytes);
                has_data = 1;
                //DLOG(INFO,"%d",bytes);
            }   
        }
        return has_data;
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
            sockaddr addr{};
            socklen_t addrlen = sizeof(addr);
            ev.data.fd = ::accept(this->srv_fd,&addr,&addrlen);
            if (ev.data.fd == -1) {
                if (errno == EAGAIN|| errno == EWOULDBLOCK) break; 
                continue;
            }
            ev.events = EPOLLIN | EPOLLET;
            fcntl(ev.data.fd, F_SETFL,fcntl(ev.data.fd,  F_GETFL)| O_NONBLOCK); 
            epoll_ctl(epoll_fd,EPOLL_CTL_ADD,ev.data.fd,&ev);
            clients[ev.data.fd] = addr;
            DLOG(CONN,"fd = %d",ev.data.fd);
        }
    }

    void handle_client(int fd) {
        std::string packet{};
        if(read(fd,packet))
        {
            //DLOG(INFO,"%u",packet.size());
            handle_stream(fd,packet);
        }
    }


    //读取报文，因为不同协议读取报文的流程不同，这里使用多态
    virtual void handle_stream(int fd,std::string packet)
    {
        int ret = write(fd,packet.data(),packet.size());
    }
    

public:
    const char * addr = nullptr;
    int port = -1;
private:
    int srv_fd = -1;
    int epoll_fd  = -1;
    struct epoll_event ev,events[1024] = {0};
    bool stop = true;

protected:
    std::map<int,sockaddr> clients;
    threadpool & pools;
};