#include <stdexcept>
#include <network/rtp_server.h>

bool SockAddrComparator::operator()(const sockaddr& a, const sockaddr& b) const {
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


rtp_server::rtp_server(threadpool &pools) : dgram_server(pools){}

void rtp_server::handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n) 
{
    if(addr == nullptr) std::runtime_error("addr is null");
    auto it = slots.find(*addr);
    if(it != slots.end())
    {
        it->second(buf,n);
    }else std::runtime_error("no match fun");
}

void rtp_server::reg_sockaddr(sockaddr *addr,size_t N,std::function<void(char *,size_t)> f)
{
    slots.emplace(*addr,f);
}