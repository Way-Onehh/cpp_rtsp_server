# cpp_rtsp_server  
![License](https://img.shields.io/badge/License-MIT-blue)  ![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey)   
 
> 基于 FFmpeg 的高性能 RTSP 流媒体服务器，支持负载均衡与多协议传输  
 
---
 
##  📌 核心特性  
- **高性能架构**  
  - 基于 Linux `epoll` I/O 复用模型，支持高并发连接  
  - 事件驱动设计，低资源占用  
- **协议支持**  
  - RTP over UDP（低延迟）  
  - RTP over TCP（可靠传输）  
- **扩展能力**  
  - 基础负载均衡逻辑，可横向扩展  
  - 无转码设计，依赖 FFmpeg 编解码  
 
---
 
##  📌 快速开始
###  推流命令（FFmpeg）  
#### **摄像头+麦克风推流(Windows 示例)**  
ffmpeg -f dshow -i video="Chicony USB2.0 Camera":audio="麦克风 (COLORFIRE H10)" \  
       -c:v libx264 -preset ultrafast -tune zerolatency \  
       -c:a aac -f rtsp <-rtsp_transport tcp> rtsp://127.0.0.1:8554/live/test  
 
#### **屏幕捕获+音频推流**  
ffmpeg -f gdigrab -i desktop -f dshow -i audio="麦克风 (COLORFIRE H10)" \  
       -c:v libx264 -preset ultrafast -tune zerolatency \  
       -c:a aac -f rtsp <-rtsp_transport tcp> rtsp://127.0.0.1:8554/live/test  
###  拉流与点播
#### **实时流播放**
ffplay <-rtsp_transport tcp> rtsp://127.0.0.1:8554/live/test  
 
#### **点播视频（需将文件放入 data 目录 eg: test1**  
ffplay <-rtsp_transport tcp> rtsp://127.0.0.1:8554/record/test1  

---

##  📌注意
udp功能只是配局域网