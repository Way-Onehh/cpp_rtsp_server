#pragma once 

#include <utility/factory.hpp>
#include <cstddef>
#include <cstdint>
#include <memory.h>
#include <memory>
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
    
public: 
    factory<sockaddr, signal<std::pair<std::shared_ptr<uint8_t[]>,size_t>>,SockAddrComparator> factory_signal;
};
