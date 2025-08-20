#pragma once 

#include <map>
#include <memory.h>

#include <functional>
#include <sys/socket.h>

#include <network/dgram_server.h>

struct SockAddrComparator {
    bool operator()(const sockaddr& a, const sockaddr& b) const ;
};

class rtp_server:  public dgram_server
{
public:
    rtp_server(threadpool &pools);
    void handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n) override;
    
    void reg_sockaddr(sockaddr *addr,size_t N,std::function<void(char *,size_t)> f);

public: 
    //返回值暂时为void
    std::map<sockaddr,std::function<void(char *,size_t)>,SockAddrComparator> slots;
};
