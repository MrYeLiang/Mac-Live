//
//  RtmpStreamEncoder.hpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/23.
//  Copyright © 2019 叶亮. All rights reserved.
//
#pragma once
struct AVFrame;
struct AVPacket;
class AVCodecContext;

//音视频编码接口类
class RtmpStreamEncoder
{
public:
    //输入参数
    int inWidth = 1280;
    int inHeight = 720;
    int inPixSize = 3;
    
    //输出参数
    int outWidth = 1280;
    int outHeight = 720;
    int bitRate = 400000; //压缩后每秒视频的bit位 50kb 50*1000*8
    int fps = 25;
    
    //工厂生产方法
    static RtmpStreamEncoder *Get(unsigned char index = 0);
    
    //初始化像素格式转换的上下文
    virtual bool InitScale() = 0;
    
    virtual AVFrame* RGBToYUV(char *rgb) = 0;
    
    //视频编码器初始化
    virtual bool InitVideoCodec() = 0;
    
    //视频编码
    virtual AVPacket * EncodeVideo(AVFrame* frame) = 0;
    
    virtual ~RtmpStreamEncoder();
    
    //编码器上下文
    AVCodecContext *codecContext = 0;
    
protected:
    RtmpStreamEncoder();
};

/* RtmpStreamEncoder_hpp */
