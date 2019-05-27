//
//  OpenCvCamera.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/22.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "OpenCvCamera.hpp"
#include "RtmpStreamEncoder.hpp"
#include "RtmpPusher.hpp"


#include <iostream>
#include <opencv2/highgui.hpp>

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
    //nginx-rtmp 直播服务器rtmp推流url
    char *outUrl = "rtmp://localhost:1935/live";
    
    //编码器和像素格式转换
    RtmpStreamEncoder *encoder = RtmpStreamEncoder::Get(0);
    
    //封装packet和推流对象
    RtmpPusher *pusher = RtmpPusher::Get(0);
    
    VideoCapture cam;
    namedWindow("video");
    Mat frame;
    
    int result = 0;
    try
    {
        //1 使用OpenCv打开相机
        cam.open(0);
        if(!cam.isOpened()){
            cout << "camera open failed！" << endl;
            return -1;
        }
        cout << "camera open success！" << endl;

        //2初始化输出的数据结构
        int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
        int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
        int fps = cam.get(CAP_PROP_FPS);

        encoder->inWidth = inWidth;
        encoder->inHeight = inHeight;
        encoder->outWidth = inWidth;
        encoder->outHeight = inHeight;

        encoder->InitScale();

        //3 初始化编码器上下文
        if(!encoder->InitVideoCodec()){
            cout << "InitVideoCodec failed" << endl;
            return -1;
        }

        pusher->Init(outUrl);

        //添加视频流
        pusher->AddStream(encoder->codecContext);
        pusher->SendHead();

        for(;;)
        {
            //读取rtsp视频帧
            if(!cam.grab())
            {
                continue;
            }

            if(!cam.retrieve(frame)){
                continue;
            }

            encoder->inPixSize = frame.elemSize();
            AVFrame *yuvFrame  = encoder->RGBToYUV((char *)frame.data);
            if(!yuvFrame)
            {
                continue;
            }

            //h264编码
            AVPacket *packet = encoder->EncodeVideo(yuvFrame);
            if(!packet){
                continue;
            }

            pusher->SendFrame(packet);

        }
        
    }catch(exception &ex)
    {
        if(cam.isOpened())
        {
            if(cam.isOpened()){
                cam.release();
            }
            cout << ex.what() << endl;
        }
    }

    getchar();
    return 0;
}
