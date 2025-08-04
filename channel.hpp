#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
class channel
{
public:
    virtual int write(char * buf, int size)=0;
    virtual int read(char * buf, int size)=0;
};

// class tcpchannel : public channel
// {
// public:
//     //需要fd非阻塞
//     tcpchannel(int fd): _fd(fd)
//     {} 
    
//     int write(char * buf, int size)
//     {
//         return ::write(_fd,buf,size);
//     }
    
//     //
//     int read(char * buf, int size)
//     {
//         return ::read(_fd,buf,size);
//     }
// private:
//     int _fd;
//     mutex mtx;
// };

// class udpchannel : public channel
// {
// public:
//     udpchannel(sockaddr addr,socklen_t socklen): addr(addr), socklen(socklen){} 
    
//     int write(char * buf, int size)
//     {
//         return ::write();
//     }
    
//     //
//     int read(char * buf, int size)
//     {
//         return ::read(_fd,buf,size);
//     }
// private:
//     int fd;
//     sockaddr  addr{};
//     socklen_t socklen =sizeof(addr);
// };