#include <stdexcept>
#include <network/dgram_server.h>

dgram_server::dgram_server(threadpool & polls)  : polls(polls) , stop(false)
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
void dgram_server::bind(std::string_view addr,int port) 
{ 
    struct sockaddr_in server_addr = {AF_INET,htons(port),0,0};
    inet_pton(AF_INET,addr.data(),&server_addr.sin_addr);
    int ret = ::bind(this->srv_fd,(struct sockaddr *) &server_addr,sizeof(server_addr));
    if(ret < 0) throw std::runtime_error("failed init bind");
    this->port = port;
    this->addr = addr.data();
}

//处理客户端
int dgram_server::handle_client()
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

void dgram_server::handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n)
{
    sendto(srv_fd,buf,n,0,addr,socklen);
}

void dgram_server::start()
{
    auto call = [this]()
    {
        while (!stop) {
            this->handle_client();
        }
    };
    polls.submit(call);
}

int& dgram_server::getfd()
{
    return  this->srv_fd;
}