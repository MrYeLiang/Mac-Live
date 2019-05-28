//
//  RtmpPusher.hpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/23.
//  Copyright © 2019 叶亮. All rights reserved.
//

#pragma once
class AVCodecContext;
class AVPacket;
class RtmpPusher
{
public:
    //工厂生产方法
    static RtmpPusher *Get(unsigned char index = 0);
    
    //初始化封装器上下文
    virtual bool Init(const char *url) = 0;

    //添加视频或者音频流
    virtual bool AddStream(const AVCodecContext *context) = 0;

    //打开rtmp网络IO 发送封装头
    virtual bool SendHead() = 0;

    //rtmp 帧推流
    virtual bool SendFrame(AVPacket *packet) = 0;
    
    virtual ~RtmpPusher();
protected:
    RtmpPusher();
};
 /* RtmpPusher_hpp */
