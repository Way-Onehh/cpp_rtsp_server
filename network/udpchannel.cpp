#include <network/udpchannel.h>
void udpchannel::set(int fd , sockaddr addr)
{
    fd_ = fd;
    addr_ = addr; 
}

ssize_t  udpchannel::send(uint8_t *buf,size_t n)
{
    socklen_t socklen = sizeof(sockaddr);
    auto bytes = sendto(fd_,buf,n,0,&addr_,socklen);
    return bytes;
}