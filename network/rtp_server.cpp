#include <cstring>
#include <memory>
#include <stdexcept>
#include <network/rtp_server.h>
#include <utility>

bool SockAddrComparator::operator()(const sockaddr& a, const sockaddr& b) const {
      // 1. 比较协议族（IPv4 < IPv6 < 其他）
        if (a.sa_family  != b.sa_family) 
            return a.sa_family  < b.sa_family; 
 
        // 2. 按协议族细化比较
        switch (a.sa_family)  {
            case AF_INET: {
                const auto* a_in = reinterpret_cast<const sockaddr_in*>(&a);
                const auto* b_in = reinterpret_cast<const sockaddr_in*>(&b);
                
                // 优先比较IP地址
                if (a_in->sin_addr.s_addr != b_in->sin_addr.s_addr)
                    return a_in->sin_addr.s_addr < b_in->sin_addr.s_addr;
                
                // IP相同则比较端口（注意网络字节序）
                return ntohs(a_in->sin_port) < ntohs(b_in->sin_port);
            }
            case AF_INET6: {
                const auto* a_in6 = reinterpret_cast<const sockaddr_in6*>(&a);
                const auto* b_in6 = reinterpret_cast<const sockaddr_in6*>(&b);
                
                // 优先比较IPv6地址 
                int cmp = memcmp(&a_in6->sin6_addr, &b_in6->sin6_addr, sizeof(in6_addr));
                if (cmp != 0)
                    return cmp < 0;
                
                // IP相同则比较端口 
                return ntohs(a_in6->sin6_port) < ntohs(b_in6->sin6_port);
            }
            default:
                // 其他协议族：按原始内存比较（谨慎使用）
                return memcmp(&a, &b, sizeof(sockaddr)) < 0;
        }
}


rtp_server::rtp_server(threadpool &pools) : dgram_server(pools){}

void rtp_server::handle_dgram(const sockaddr *addr, socklen_t socklen,char * buf,size_t n) 
{
    if(addr == nullptr) std::runtime_error("addr is null");
    if(factory_signal.count(*addr))
    {
        auto packet = std::make_shared<uint8_t[]>(n);
        memcpy(packet.get(), buf, n);
        factory_signal.at(*addr)->emit(std::make_pair(std::move(packet), n));
    }else std::runtime_error("no match fun");
}
