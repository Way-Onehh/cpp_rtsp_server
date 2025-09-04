#pragma once

#include <filesystem>
#include <protocol/rtsp.hpp>
#include <utility/factory.hpp>
#include <network/stream_server.h>
struct fd_ch
{
    int fd,ch;
};
bool operator<(const fd_ch& a, const fd_ch& b);

class rtsp_server : public stream_server 
{
public:
    rtsp_server(threadpool &polls,std::filesystem::path workpath = std::filesystem::current_path());
    //绑定端口
    void bind(std::string_view addr,std::initializer_list<int> list);
    //开处理rtsp rtp rtcp 信息
    void start();
    //设置本地点播流的参数
    void set(uint32_t video_time_base,uint32_t video_frame_size,uint32_t audio_time_base,uint32_t audio_frame_size);
private:
    void handle_stream(int fd) override;

    void handle_rtsp(int fd);

    void handle_rtp_rtcp(int fd);

    bool handle_request(int fd,request &req);

    bool handle_OPTIONS(int fd,request &req);

    bool handle_DESCRIBE(int fd, request &req);

    bool handle_ANNOUNCE(int fd,request &req);

    bool handle_SETUP(int fd, request &req);

    bool handle_PLAY(int fd, request &req);

    bool handle_RECORD(int fd, request &req);

    bool handle_TEARDOWN(int fd, request &req);

    std::shared_ptr<server_config> getcofig(int fd);

    bool setup(int fd,std::string root,std::string streamname,int port1_or_channel_n1,int port2_or_channel_n2);

    bool setup_pull_stream(std::shared_ptr<session> session_obj ,const std::function<void(std::string)> & finish_slot, int fd,std::string root,std::string_view streamname,int port1_or_channel_n1,int port2_or_channel_n2);

    bool setup_push_stream(
        std::shared_ptr<session> session_obj ,const std::function<void(std::string)> & finish_slot,
        int fd,std::string root,std::string_view streamname,int port1_or_channel_n1,int port2_or_channel_n2);

public:
    std::vector<std::shared_ptr<server_config>>                         cfgs;                       //封装udp tcp 配置
    int                                                                 current_session_index = 0;  //id索引
    std::filesystem::path                                               workpath;                   //工作目录
    factory<std::string,stream>                                         stream_factory;             //流工厂
    factory<int,session>                                                sessions_factory;           //会话工厂
    factory<std::string,sdp::MediaDescription>                          mdp_factory;                //媒体描述符工厂
    factory<fd_ch,signal<std::pair<std::shared_ptr<uint8_t[]>,size_t>>> factory_signal;             //信号工厂
    uint32_t                                                            video_time_base_ = 1200000; //流的参数
    uint32_t                                                            video_frame_size_ = 40000;  //流的参数
    uint32_t                                                            audio_time_base_ = 44100;   //流的参数
    uint32_t                                                            audio_frame_size_ = 1024;   //流的参数
    std::string                                                         raw_packet;                 //tcp 流数据
    int                                                                 index = 0;                  //raw_packet索引
};
