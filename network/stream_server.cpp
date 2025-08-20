#include <network/stream_server.h>

std::function<void(int)> stream_server::handle_close;
std::map<int,sockaddr> stream_server::clients;
int stream_server::epoll_fd = -1;


stream_server::stream_server(threadpool & polls) :pools(polls),stop(false)
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

stream_server::~stream_server()
{
    stop        =   true;
    close(this->epoll_fd);
    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,srv_fd,0);
    close(this->srv_fd);
}

//绑定地址
void stream_server::bind(std::string_view addr,int port) 
{ 
    struct sockaddr_in server_addr  =   {AF_INET,htons(port),0,0};
    inet_pton(AF_INET,addr.data(),&server_addr.sin_addr);
    int ret                         =   ::bind(this->srv_fd,(struct sockaddr *) &server_addr,sizeof(server_addr));
    if(ret < 0) throw std::runtime_error("failed init bind");
    this->port = port;
    this->addr = addr.data();
}

//开始监听
void stream_server::listen()
{
    int ret = ::listen(this->srv_fd,128);
    if(ret < 0) throw std::runtime_error("failed init listen");
}

//
int stream_server::accept_epoll()
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

void stream_server::start()
{   
    
    auto obj =  std::enable_shared_from_this<stream_server>::shared_from_this();
    auto call = [obj]()
    { 
        obj->accept_epoll();
        if(!obj->stop)
        obj->pools.submit(std::bind(&stream_server::start,obj));
    };
    pools.submit(call);
}

//当发生拆包时，再次调用这个函数追加剩余数据
bool stream_server::read(int fd,std::string &packet)
{
    char buf[1024]{};
    bool has_data = 0;
    int times = 0;
    while (true) {
        memset(buf,0,1024);
        int bytes = ::read(fd, buf, sizeof(buf)); // 动态缓冲区大小
        if (bytes == -1 &&(errno == EAGAIN || errno == EWOULDBLOCK)) break; 
        else if (bytes <= 0) { 
            if(handle_close)
                handle_close(fd);
            break;
        } 
        else {
            packet.append(buf, bytes);
            has_data = 1;
        }   
    }
    return has_data;
}


//使用epoll IO复用
void stream_server::doepoll(int fd , int event)
{
    if(fd == this->srv_fd && event == EPOLLIN)  {handle_accept();    return;}
    if(event == EPOLLIN)                         handle_client(fd);   
}

//接受新的连接，使用epoll方法
void stream_server::handle_accept()
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

void stream_server::handle_client(int fd) {
    std::string packet{};
    if(read(fd,packet))
    {
        //DLOG(INFO,"%u",packet.size());
        handle_stream(fd,packet);
    }
}


//读取报文，因为不同协议读取报文的流程不同，这里使用多态
void stream_server::handle_stream(int fd,std::string packet)
{
    int ret = write(fd,packet.data(),packet.size());
}

