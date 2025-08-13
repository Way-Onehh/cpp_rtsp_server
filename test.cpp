#include "H264_frame_generator.hpp"
#include "H264_packetizer.hpp"
#include "rtp.h"
#include "rtp_packetizer.h"
#include <iostream>
#include <ostream>
#include <sdp.hpp>
namespace test_utils  {
    auto test_H264_frame_generator() 
    {
        H264_frame_generator fg;
        fg.open("../test.h264");
        int times = 0;
        while (fg && times< 10) {
            auto &&ret = fg();
            std::cout<<ret.second<<std::endl;
            times ++;
        }
    }

    auto test_H264_packetizer()
    {
        rtp_packetizer <H264_frame_generator,H264_packetizer> a;
        a.frame_generator.open("../test.h264");
        a.packetizer.set(100, 10);
        uint8_t rtp_data[1400] {};
        int times = 0;
        auto bytes = a.next_packet(rtp_data,1400);
        while (bytes && times< 20) {
            rtp_header * header =(rtp_header *) rtp_data;
                std::cout << "RTP[v:" << static_cast<int>(header->version)  
              << " p:" <<  static_cast<bool>(header->padding)
              << " x:" <<  static_cast<bool>(header->extension)
              << " cc:" << static_cast<int>(header->csrc_count)  
              << " m:" <<  static_cast<bool>(header->marker)
              << " pt:" << static_cast<int>(header->payload_type) 
              << " seq:" <<static_cast<int>(header->sequence_number )  
              << " ts:" << static_cast<int>(header->timestamp )  
              << " ssrc:" << header->ssrc  << std::dec 
              << "]\n";
            bytes = a.next_packet(rtp_data,1400);
            times ++;
        }
    }

    auto test_sdp_info()
    {
        std::string result = sdp::generate_fmtp_from_h264_file("../test.h264");
        if (!result.empty())  {
            std::cout << "Generated SDP fmtp string:\n" << result << std::endl;
        } else {
            std::cerr << "Failed to generate SDP string (invalid file or no SPS/PPS found)" << std::endl;
        }
        return 0;
    }

    auto test_alignas()
    {
        std::cout<<sizeof(rtp_header)<<" "<<sizeof(fu_header)<<" "<<sizeof(fu_indicator)<<std::endl;
        return 0;
    }
}


int main()
{
    test_utils::test_alignas();
}