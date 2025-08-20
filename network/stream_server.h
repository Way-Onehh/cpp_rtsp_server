#pragma once

#include <functional>
#include <map>
#include <memory.h>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#include <utility/log.hpp>
#include <utility/threadpool.hpp>

class stream_server :public std::enable_shared_from_this<stream_server>
{
public: 
    stream_server(threadpool & polls);

    ~stream_server();

    //绑定地址
    void bind(std::string_view addr,int port);

    //开始监听
    void listen();
    
    //
    int accept_epoll();

    void start();

    //当发生拆包时，再次调用这个函数追加剩余数据
    bool read(int fd,std::string &packet);

private:
    
    //使用epoll IO复用
    void doepoll(int fd , int event);
    
    //接受新的连接，使用epoll方法
    void handle_accept();

    void handle_client(int fd);

    //读取报文，因为不同协议读取报文的流程不同，这里使用多态
    virtual void handle_stream(int fd,std::string packet);
public:
    const char * addr = nullptr;
    int port = -1;
    static std::map<int,sockaddr> clients;
    threadpool & pools;
    static std::function<void(int)> handle_close; 
    static  int epoll_fd;
private:
    int srv_fd = -1;
    struct epoll_event ev,events[1024] = {0};
    bool stop = true;
protected:

};

#define  CLOSE_SLOT_START         stream_server::handle_close =[this](int fd)        { static std::mutex  mtx;  std::lock_guard lg(mtx);
#define  CLOSE_SLOT_END           ::close(fd);DLOG(DISCONN, "fd: %d", fd);::epoll_ctl(stream_server::epoll_fd, EPOLL_CTL_DEL, fd, nullptr);stream_server::clients.erase(fd);};