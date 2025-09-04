#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <network/tcpchannel.h>
#include <sys/types.h>
#include <network/stream_server.h>
#include <network/server_config.h>

std::mutex tcpchannel::mtx;

void tcpchannel::set(int fd , void * ch_n_)
{
    fd_ = fd;
    ch_n = *(int*)ch_n_;
}

ssize_t tcpchannel::send(uint8_t *buf,size_t n)
{   
    if(is_close)return 0;
    size_t size =  n+4; 
    auto tcp_buf =std::make_shared<uint8_t[]>(size);
    tcp_buf[0] = 0x24;//$
    tcp_buf[1] = ch_n;// 0x00;
    tcp_buf[2] = (uint8_t)(((n) & 0xFF00) >> 8);
    tcp_buf[3] = (uint8_t)((n) & 0xFF);
    memcpy(tcp_buf.get()+4,buf, n);
    ssize_t bytes = -1;
    
    {
        std::lock_guard lg(tcpchannel::mtx);
        bytes = ::send(fd_,tcp_buf.get(),size,MSG_NOSIGNAL);
    }

    if(bytes < 0 ) 
    {
        ON_CLOSE_CLIENT->emit(fd_);
        close();  
    }
    return bytes;
}

void tcpchannel::close()
{
    on_close.emit(fd_,nullptr);
    this->is_close = 1;
}