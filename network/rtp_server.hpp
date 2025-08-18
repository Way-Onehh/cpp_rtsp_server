#pragma once 

#include <map>
#include <memory.h>
#include <stdexcept>
#include <functional>
#include <sys/socket.h>

#include <network/dgram_server.hpp>

struct SockAddrComparator {
    bool operator()(const sockaddr& a, const sockaddr& b) const {
        if (a.sa_family  != b.sa_family) 
            return a.sa_family  < b.sa_family; 
 
        switch (a.sa_family)  {
            case AF_INET: {
                const sockaddr_in* a_in = reinterpret_cast<const sockaddr_in*>(&a);
                const sockaddr_in* b_in = reinterpret_cast<const sockaddr_in*>(&b);
                return memcmp(&a_in->sin_addr, &b_in->sin_addr, sizeof(a_in->sin_addr)) < 0;
            }
            case AF_INET6: {
                const sockaddr_in6* a_in6 = reinterpret_cast<const sockaddr_in6*>(&a);
                const sockaddr_in6* b_in6 = reinterpret_cast<const sockaddr_in6*>(&b);
                return memcmp(&a_in6->sin6_addr, &b_in6->sin6_addr, sizeof(a_in6->sin6_addr)) < 0;
            }
            default:
                return memcmp(a.sa_data,  b.sa_data,  sizeof(a.sa_data))  < 0;
        }
    }
};

class rtp_server:  public dgram_server
{
public:
    rtp_server(threadpool &pools) : dgram_server(pools){}
    void handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n) override
    {
        if(addr == nullptr) std::runtime_error("addr is null");
        auto it = slots.find(*addr);
        if(it != slots.end())
        {
            it->second(buf,n);
        }else std::runtime_error("no match fun");
    }
    
    void reg_sockaddr(sockaddr *addr,size_t N,std::function<void(char *,size_t)> f)
    {
        slots.emplace(*addr,f);
    }

public: 
    //返回值暂时为void
    std::map<sockaddr,std::function<void(char *,size_t)>,SockAddrComparator> slots;
};
