#include "AAC_frame_generator.hpp"
#include "AAC_packetizer.hpp"
#include "H264_frame_generator.hpp"
#include "H264_packetizer.hpp"
#include "rtp.h"
#include "rtp_packetizer.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sdp.hpp>
#include <acc.h>
#include <fstream>
#include <sys/types.h>
#include <vector>
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

    auto test_aac_header()
    {
        std::fstream f;
        std::vector<uint8_t> data;
        f.open("/mnt/d/wr/rtsp/test.aac");
        data = std::vector<uint8_t>(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
        adts_header header{};
        memcpy(header ,data.data(), 7);
        printf("syncword:          0x%03X\n", SYNCWORD(header));
        printf("ID:                %u\n", ID(header));
        printf("layer:             %u\n", LAYER(header));
        printf("protection_absent: %u\n", PROTECTION_ABSENT(header));
        printf("profile:           %u\n", PROFILE(header));
        printf("sampling_freq_idx: %u\n", SAMPLE_FREQ_IDX(header));
        printf("private_bit:       %u\n", PRIVATE_BIT(header));
        printf("channel_config:    %u\n", CHANNEL_CONFIG(header));
        printf("original_copy:     %u\n", ORIGINAL_COPY(header));
        printf("home:              %u\n", HOME(header));
        printf("copyright_id_bit:  %u\n", COPYRIGHT_ID_BIT(header));
        printf("copyright_id_start:%u\n", COPYRIGHT_ID_START(header));
        printf("frame_length:      %u\n", FRAME_LENGTH(header));
        printf("buffer_fullness:   0x%03X\n", BUFFER_FULLNESS(header));
        printf("num_raw_data_blocks: %u\n", NUM_RAW_BLOCKS(header));
        return 0;
    }

        auto test_AAC_frame_generator_header()
    {
        AAC_frame_generator fg;
        fg.open("/mnt/d/wr/rtsp/test.aac");
        int times = 0;
        while (fg && times< 20) {
            adts_header header{};
            auto packet = fg();
            memcpy(header ,packet.first.get(), 7);
            printf("____packet size:%lu____\n", packet.second);
            printf("syncword:          0x%03X\n", SYNCWORD(header));
            printf("ID:                %u\n", ID(header));
            printf("layer:             %u\n", LAYER(header));
            printf("protection_absent: %u\n", PROTECTION_ABSENT(header));
            printf("profile:           %u\n", PROFILE(header));
            printf("sampling_freq_idx: %u\n", SAMPLE_FREQ_IDX(header));
            printf("private_bit:       %u\n", PRIVATE_BIT(header));
            printf("channel_config:    %u\n", CHANNEL_CONFIG(header));
            printf("original_copy:     %u\n", ORIGINAL_COPY(header));
            printf("home:              %u\n", HOME(header));
            printf("copyright_id_bit:  %u\n", COPYRIGHT_ID_BIT(header));
            printf("copyright_id_start:%u\n", COPYRIGHT_ID_START(header));
            printf("frame_length:      %u\n", FRAME_LENGTH(header));
            printf("buffer_fullness:   0x%03X\n", BUFFER_FULLNESS(header));
            printf("num_raw_data_blocks: %u\n", NUM_RAW_BLOCKS(header));
            times++;
        }
        
        return 0;
    }

        auto test_AAC_packetizer()
    {
        rtp_packetizer <AAC_frame_generator,AAC_packetizer> a;
        a.frame_generator.open("/mnt/d/wr/rtsp/test.aac");
        a.packetizer.set(100, 10);
        uint8_t rtp_data[1400] {};
        int times = 0;
        auto bytes = a.next_packet(rtp_data,1400);
        while (bytes && times< 20) {
            bytes = a.next_packet(rtp_data,1400);
            times ++;
        }
    }
}


int main()
{
    test_utils::test_AAC_packetizer();
}