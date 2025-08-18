#include <cstdint>
#include <sys/socket.h>
class channel
{
public:
    virtual void set(int fd , sockaddr addr) = 0;
    virtual ssize_t send(uint8_t * buf,size_t n) = 0;
};
