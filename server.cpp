#include<rtsp_server.hpp>
#include<threadpool.hpp>
#include<iostream>
#include<rtsp.hpp>
#include<log.hpp>
int main(int argc, char const *argv[])
{   
    try
    {
        rtsp_server srv;
        srv.bind("0.0.0.0",8881);
        srv.listen();
        DLOG(INFO,"server listen at %s:%d",srv.addr,srv.port);
        threadpool polls(12);
        DLOG(INFO,"%s","threadpoll start");
        while (1)    
        {   
            auto ret = polls.submit(std::bind(&stream_server::accept,  &srv));
            ret.wait();
        }
    }
    catch(const std::exception& e)
    {
        DLOG(EXCEP,"%s",e.what());
    }
}

// #include<dgram_server.hpp>
// #include<threadpool.hpp>
// #include<iostream>
// #include<log.hpp>
// int main(int argc, char const *argv[])
// {   
//     try
//     {
//         dgram_server srv;
//         srv.bind("0.0.0.0",8881);
//         DLOG(INFO,"server listen at %s:%d",srv.addr,srv.port);
//         threadpool polls(12);
//         DLOG(INFO,"%s","threadpoll start");
//         while (1)    
//         {   
//             auto ret = polls.submit(std::bind(&dgram_server::handle_client,  &srv));
//             ret.wait();
//         }
//     }
//     catch(const std::exception& e)
//     {
//         DLOG(EXCEP,"%s",e.what());
//     }
// }

// #include <sdp.hpp>
// int main() {
//     SDP sdp;
//     std::string sdpText = 
//         "v=0\r\n"
//         "o=alice 2890844526 2890844526 IN IP4 host.example.com\r\n" 
//         "s=Example SDP\r\n"
//         "c=IN IP4 host.example.com\r\n" 
//         "t=0 0\r\n"
//         "m=audio 49170 RTP/AVP 0\r\n"
//         "a=rtpmap:0 PCMU/8000\r\n"
//         "m=video 51372 RTP/AVP 99\r\n"
//         "a=rtpmap:99 H264/90000\r\n";
    
//     if (sdp.parse(sdpText))  {
//         std::cout << "SDP parsed successfully!" << std::endl;
        
//         // 序列化并打印 
//         std::cout << "\nSerialized SDP:\n" << sdp.serialize()  << std::endl;
        
//         // 序列化为 char*
//         char* sdpChar = sdp.serializeToCharPtr(); 
//         std::cout << "\nAs char*:\n" << sdpChar << std::endl;
//         delete[] sdpChar;
//     } else {
//         std::cerr << "Failed to parse SDP!" << std::endl;
//     }
    
//     return 0;
// }


// #include <algorithm>
// #include <iostream>
// #include <format.hpp>
// using namespace std;
// int main()
// {
//     SDP sdp;
//     sdp.parse(R"(v=0
// o=- 0 0 IN IP6 ::1
// s=No Name
// c=IN IP6 ::1
// t=0 0
// a=tool:libavformat 61.7.100
// m=video 0 RTP/AVP 96
// a=rtpmap:96 H264/90000
// a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z0LAH9oBQBboQAAAAwBAAAAMo8YMqA==,aM4PyA==; profile-level-id=42C01F
// a=control:streamid=0
// m=audio 0 RTP/AVP 97
// b=AS:128
// a=rtpmap:97 MPEG4-GENERIC/44100/2
// a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=121056E500
// a=control:streamid=1)");
//     format fmt(sdp);
//     return 0;
// };
