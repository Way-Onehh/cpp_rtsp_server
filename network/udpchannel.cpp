#include <network/udpchannel.h>
#include <sys/socket.h>
void udpchannel::set(int fd , void * addr)
{
    fd_     = fd;
    addr_   = *(sockaddr *) addr; 
}

ssize_t  udpchannel::send(uint8_t *buf,size_t n)
{
    socklen_t socklen = sizeof(sockaddr);
    auto bytes = sendto(fd_,buf,n,0,&addr_,socklen);
    return bytes;
}

void udpchannel::close(){}