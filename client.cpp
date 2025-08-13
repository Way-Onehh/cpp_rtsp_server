extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}


int main() {
    // 初始化 FFmpeg 
    avformat_network_init();  // 启用网络协议支持[1]()[3]()
 
    AVFormatContext *fmt_ctx = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVPacket pkt;
    AVFrame *frame = av_frame_alloc();
    FILE *yuv_file = fopen("output.yuv",  "wb");
 
    // 打开 RTSP 流（示例URL）
    const char *url = "rtsp://localhost:8554/mystream";
    if (avformat_open_input(&fmt_ctx, url, NULL, NULL) != 0) {
        fprintf(stderr, "无法打开输入流\n");
        return -1;
    }
 
    // 获取流信息 
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "无法获取流信息\n");
        return -1;
    }
 
    // 查找视频流索引
    int video_stream_idx = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) {
        fprintf(stderr, "未找到视频流\n");
        return -1;
    }
 
    // 初始化解码器
    AVCodecParameters *codec_par = fmt_ctx->streams[video_stream_idx]->codecpar;
    codec = avcodec_find_decoder(codec_par->codec_id);  // 支持 H264/H265[8]()[3]()
    if (!codec) {
        fprintf(stderr, "不支持的编解码器\n");
        return -1;
    }
 
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_par);
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "无法打开解码器\n");
        return -1;
    }
 
    // 主循环：读取并解码帧 
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index  == video_stream_idx) {
            // 发送数据包到解码器 
            if (avcodec_send_packet(codec_ctx, &pkt) == 0) {
                // 接收解码后的帧
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // 写入 YUV 数据（YUV420P格式）
                    for (int i = 0; i < 3; i++) {  // 遍历 Y、U、V 分量
                        int width = (i == 0) ? frame->width : frame->width / 2;
                        int height = (i == 0) ? frame->height : frame->height / 2;
                        uint8_t *data = frame->data[i];
                        
                        for (int h = 0; h < height; h++) {
                            fwrite(data, 1, width, yuv_file);
                            data += frame->linesize[i];
                        }
                    }
                }
            }
        }
        av_packet_unref(&pkt);  // 释放数据包[1]()[10]()
    }
 
    // 清理资源 
    fclose(yuv_file);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    avformat_network_deinit();
    return 0;
}