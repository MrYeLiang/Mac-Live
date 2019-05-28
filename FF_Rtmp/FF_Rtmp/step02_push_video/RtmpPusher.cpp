//
//  RtmpPusher.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/23.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "RtmpPusher.hpp"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <string>
#include <iostream>

using namespace std;

class RealRtmpPusher : public RtmpPusher{
public:
    void Close()
    {
        if (formatContext)
        {
            avformat_close_input(&formatContext);
            avStream = NULL;
        }

        formatContext = NULL;
        codecContext = NULL;
        url = "";
    }

    bool Init(const char *url)
    {
        //输出封装器和视频流配置
        //a 创建输出封装器上下文
        int result = avformat_alloc_output_context2(&formatContext, 0, "flv", url);
        this->url = url;
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            cout << buf << endl;
            return false;
        }
        return true;
    }

    bool AddStream(const AVCodecContext *context)
    {
        if (!context)
        {
            return false;
        }

        //添加视频流
        AVStream *stream = avformat_new_stream(formatContext, NULL);
        if(!stream)
        {
            cout << "avformat_new_stream failed!" << endl;
            return false;
        }

        stream->codecpar->codec_tag = 0;

        //从编码器复制参数
        avcodec_parameters_from_context(stream->codecpar, context);

        av_dump_format(formatContext, 0, url.c_str(), 1);

        if(context->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            codecContext = context;
            avStream = stream;
        }
        return true;
    }

    bool SendHead()
    {
        //打开rtmp的网络输出IO
        int result = avio_open(&formatContext->pb, url.c_str(), AVIO_FLAG_WRITE);
        if(result != 0)
        {
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf) - 1);
            cout << buf << endl;
            return false;
        }

        //写入封装头
        result = avformat_write_header(formatContext, NULL);
        if(result != 0)
        {
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf) -1);
            cout << buf << endl;
            return false;
        }
        return true;
    }

    bool SendFrame(AVPacket *packet)
    {
        if (packet->size <= 0 || !packet->data){
            return false;
        }
        //推流
        packet->pts = av_rescale_q(packet->pts, codecContext->time_base, avStream->time_base);
        packet->dts = av_rescale_q(packet->dts, codecContext->time_base, avStream->time_base);

        packet->duration = av_rescale_q(packet->duration, codecContext->time_base, avStream->time_base);
        int result = av_interleaved_write_frame(formatContext, packet);
        if (result == 0){
            cout << "#" << flush;
            return true;
        }

        return false;
    }
    
private:
    //rtmp flv 封装器
    AVFormatContext *formatContext = NULL;
    
    //视频编码器流
    const AVCodecContext *codecContext = NULL;
    
    AVStream *avStream = NULL;
    
    string url = "";
};

RtmpPusher *RtmpPusher::Get(unsigned char index)
{
    static RealRtmpPusher realPusher[255];
    static bool isFirst = true;
    if(isFirst)
    {
        //注册所有的封装器
        av_register_all();
        
        //注册所有的网络协议
        avformat_network_init();
        isFirst = false;
    }
    return &realPusher[index];
}

RtmpPusher:: ~RtmpPusher(){}
RtmpPusher::RtmpPusher(){}
