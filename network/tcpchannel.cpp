#include <cstring>
#include <fcntl.h>
#include <memory>
#include <network/tcpchannel.h>
#include <sys/types.h>
#include <network/stream_server.h>
void tcpchannel::set(int fd , void * ch_n_)
{
    fd_ = fd;
    ch_n = *(int*)ch_n_;
}

ssize_t tcpchannel::send(uint8_t *buf,size_t n)
{   
    if(is_close) return 0;
    size_t size =  n+4;
    auto tcp_buf =std::make_shared<uint8_t[]>(size);
    tcp_buf[0] = 0x24;//$
    tcp_buf[1] = ch_n;// 0x00;
    tcp_buf[2] = (uint8_t)(((n) & 0xFF00) >> 8);
    tcp_buf[3] = (uint8_t)((n) & 0xFF);
    memcpy(tcp_buf.get()+4,buf, n);

    //std::lock_guard lg1(reg_watch[fd_].first);
    if(is_close) return 0;
    ssize_t bytes = ::send(fd_,tcp_buf.get(),size,MSG_NOSIGNAL);

    if(bytes < 0 ) 
    {
        close();
        stream_server::handle_close(fd_);
    }
    return bytes;
}

void tcpchannel::close()
{
    this->is_close = 1;
}