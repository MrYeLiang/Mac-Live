//
//  OpenCvCamera.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/22.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "OpenCvCamera.hpp"
#include <iostream>
#include <exception>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

extern "C"
{
    #include <libswscale/swscale.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}


#pragma comment(lib,"opencv_world320.lib“)
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avformat.h")

using namespace std;
using namespace cv;

int OpenCvCamera::capture()
{
    VideoCapture cam;
    namedWindow("video");
    
    //nginx-rtmp 直播服务器rtmp推流url
    char *outUrl = "rtmp://localhost:1935/live";
    
    //像素格式转换上下文
    SwsContext *swsContext = NULL;
    
    //输出的数据结构
    AVFrame *yuvFrame = NULL;
    
    //编码器上下文
    AVCodecContext *codecContext = NULL;
    
    //rtmp flv 封装器
    AVFormatContext *formatContext = NULL;
    
    try
    {
        //1 使用OpenCv打开摄像机
        cam.open(0);
        if(!cam.isOpened()){
            cout << "cam Open failed!" << endl;
            return -1;
        }
        cout << "Camera open success!" << endl;
        
        int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
        int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
        int fps = cam.get(CAP_PROP_FPS);
        
        //2初始化格式转换上下文
        swsContext = sws_getCachedContext(swsContext,
                                         inWidth, inHeight, AV_PIX_FMT_BGR24,
                                         inWidth, inHeight, AV_PIX_FMT_YUV420P,
                                          SWS_BICUBIC, 0,0,0);
        
        if(!swsContext){
              cout << "sws_getCachedContext failed!" << endl;
            return -1;
        }
        
        //3 初始化输出的数据结构
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
            return -1;
        }
        
        //4 初始化编码上下文
        //a---------找到编码器
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if(!codec){
            cout << "Can't find h264 encoder" << endl;
            return -1;
        }
        
        //b--------创建编码器上下文
        codecContext = avcodec_alloc_context3(codec);
        if(!codecContext){
            cout << "avcodec_alloc_context3 failed" << endl;
        }
        
        //c--------配置编码器参数
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//全局s参数
        codecContext->codec_id = codec->id;
        codecContext->thread_count = 8;
        
        codecContext->bit_rate = 50 * 1024 *8; //压缩后每秒视频的bit位大小为50kb
        codecContext->width = inWidth;
        codecContext->height = inHeight;
        codecContext->time_base = {1 ,fps};
        codecContext->framerate = {fps, 1};
        
        //画面组的大小， 多少帧一个关键帧
        codecContext->gop_size = 50;
        codecContext->max_b_frames = 0;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        
        //d-------打开编码器上下文
        result  = avcodec_open2(codecContext, 0, 0);
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            return -1;
        }
        
        //5==============输出封装器和视频流配置
        //a--------创建封装器上下文
        result = avformat_alloc_output_context2(&formatContext, 0, "flv", outUrl);
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            return -1;
        }
        
        //b--------添加视频流
        AVStream *vStream = avformat_new_stream(formatContext, NULL);
        if(!vStream){
            cout << "avformat_new_stream failed" << endl;
        }
        
        vStream->codecpar->codec_tag = 0;
        //c-------从编码器复制参数
        avcodec_parameters_from_context(vStream->codecpar, codecContext);
        av_dump_format(formatContext, 0, outUrl, 1);
        
        //d--------打开rtmp的网络输出IO
        result = avio_open(&formatContext->pb, outUrl, AVIO_FLAG_WRITE);
        
        //e--------写入封装头
        result = avformat_write_header(formatContext, NULL);
        if(result != 0){
            char buf[1024] = {0};
            av_strerror(result, buf, sizeof(buf)-1);
            return -1;
        }
        
        AVPacket packet;
        memset(&packet, 0, sizeof(packet));
        Mat frame;
        int vpts = 0;
        for(;;){
            //读取rtsp视频帧 解码视频帧
            if(!cam.grab()){
                continue;
            }
            
            //yuv转换为rgb
            if(!cam.retrieve(frame)){
                continue;
            }
            imshow("video", frame);
            waitKey(1);
            
            //rgb to yuv
            //输出的数据结构
            uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
            //indata[0] bgrbgrbgr
            //plane indata[0] bbbbb indata[1] ggggg indata[2] rrrrr
            indata[0] = frame.data;
            int insize[AV_NUM_DATA_POINTERS] = {0};
            
            //一行(宽) 数据的字节数
            insize[0] = frame.cols * frame.elemSize();
            int h = sws_scale(swsContext, indata, insize, 0, frame.rows, yuvFrame->data, yuvFrame->linesize);
            
            if(h <= 0){
                continue;
            }
            
            //h264编码
            yuvFrame->pts = vpts;
            vpts ++;
            result = avcodec_send_frame(codecContext, yuvFrame);
            if(result != 0){
                continue;
            }
            result = avcodec_receive_packet(codecContext, &packet);
            if(result != 0 || packet.size > 0){
                cout << "*" << packet.size << flush;
            }else{
                continue;
            }
            
            //--------推流
            packet.pts = av_rescale_q(packet.pts, codecContext->time_base, vStream->time_base);
            packet.dts = av_rescale_q(packet.dts, codecContext->time_base, vStream->time_base);
            packet.duration = av_rescale_q(packet.duration, codecContext->time_base, vStream->time_base);
            
            result = av_interleaved_write_frame(formatContext, &packet);
            if(result == 0){
                cout << "#" << flush;
            }
            
            
            
            
        }
        
        
    } catch(exception &ex)
    {
        //释放视频采集资源
        if(cam.isOpened()){
            cam.release();
        }
        
        //释放转换上下文对象
        if(swsContext)
        {
            sws_freeContext(swsContext);
            swsContext = NULL;
        }
        
        //释放编码器上下文
        if(codecContext){
            avio_closep(&formatContext->pb);
            avcodec_free_context(&codecContext);
            
        }
    
         cerr << ex.what() <<endl;
    }
    
    getchar();
    return 0;
}
