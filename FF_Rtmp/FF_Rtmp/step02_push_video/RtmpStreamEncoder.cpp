//
//  RtmpStreamEncoder.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/23.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "RtmpStreamEncoder.hpp"
extern "C"
{
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}

#include <iostream>

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

using namespace std;
class RealEncoder : public RtmpStreamEncoder{
public:
    
    //释放资源
    void Close()
    {
        if(swsContext){
            sws_freeContext(swsContext);
            swsContext = NULL;
        }
        
        if(yuvFrame)
        {
            av_frame_free(&yuvFrame);
        }
        
        if(codecContext){
            avcodec_free_context(&codecContext);
        }
        vpts = 0;
        av_packet_unref(&packet);
    }
    
   
    bool InitScale()
    {
         //1 初始化格式转换上下文
        swsContext = sws_getCachedContext(swsContext, inWidth, inHeight, AV_PIX_FMT_BGR24, outWidth, outHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
        
        if(!swsContext){
            cout << "sws_getCachedContext failed!" << endl;
            return false;
        }
        
        //2 初始化输出的数据结构
        yuvFrame = av_frame_alloc();
        yuvFrame->format = AV_PIX_FMT_YUV420P;
        yuvFrame->width = inWidth;
        yuvFrame->height = inHeight;
        yuvFrame->pts = 0;
        
        //分配yuv空间
        int result = av_frame_get_buffer(yuvFrame, 32);
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            return false;
        }
        return true;
    }
    
    AVFrame* RGBToYUV(char *rgb)
    {
        //rgb to yuv
        //输入的数据结构
        uint8_t *inData[AV_NUM_DATA_POINTERS] = {0};
        inData[0] = (uint8_t*)rgb;
        
        //indata[0] bgrbgrbgr
        //plane indata[0] bbbbb--------indata[1]ggggg--------indata[2]rrrrr
        int inSize[AV_NUM_DATA_POINTERS] = { 0 };
        
        //一行(宽) 数据的字节数
        inSize[0] = inWidth * inPixSize;
        
        int h = sws_scale(swsContext, inData, inSize, 0, inHeight, yuvFrame->data, yuvFrame->linesize);
        
        if(h <= 0){
            return NULL;
        }
        
        return yuvFrame;
    }
    
    //初始化编码上下文
    bool InitVideoCodec()
    {
        //1 查找解码器
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if(!codec){
            cout << "Can't find h264 encoder!" << endl;
            return false;
        }
        
        //2 创建编码器上下文
        codecContext = avcodec_alloc_context3(codec);
        if(!codecContext)
        {
            cout <<  "avcodec_alloc_context3 failed!"  << endl;
            return false;
        }
        
        //3 配置编码器参数
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//全局参数
        codecContext->codec_id = codec->id;
        codecContext->thread_count = 8;
        
        codecContext->bit_rate = 50 * 1024 * 8; //压缩后每秒视频的bit位z大小为50k
        codecContext->width = outWidth;
        codecContext->height = outHeight;
        codecContext->time_base = {1, fps};
        codecContext->framerate = {fps, 1};
        
        //画面组的大小 多少帧一个关键帧
        codecContext->gop_size = 50;
        codecContext->max_b_frames = 0;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        
        //4  打开编码器上下文
        int result = avcodec_open2(codecContext, 0, 0);
        if(result != 0)
        {
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            cout <<  buf << endl;
            return false;
        }
        
        cout << "avcodec_open2 success!" << endl;
        return true;
    }
    
    AVPacket * EncodeVideo(AVFrame* frame)
    {
        av_packet_unref(&packet);
        //h264编码
        frame->pts = vpts;
        vpts++;
        
        int result = avcodec_send_frame(codecContext, frame);
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            return NULL;
        }
        result = avcodec_receive_packet(codecContext, &packet);
        if(result != 0 || packet.size <= 0){
            return NULL;
        }
        return &packet;
    }
    
    
    
private:
    //像素格式转换上下文
    SwsContext *swsContext = NULL;
    
    //输出的YUV
    AVFrame *yuvFrame = NULL;
    AVPacket packet = {0};
    int vpts = 0;
};

RtmpStreamEncoder * RtmpStreamEncoder::Get(unsigned char index)
{
    static bool isFirst = true;
    if(isFirst)
    {
        //注册所有的编解码器
        avcodec_register_all();
        isFirst = false;
    }
    
    static RealEncoder encoder[255];
    return &encoder[index];
}

RtmpStreamEncoder::RtmpStreamEncoder(){}

RtmpStreamEncoder::~RtmpStreamEncoder(){}
