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
}


#pragma comment(lib,"opencv_world320.lib“)
#pragma comment(lib, "swscale.lib")

using namespace std;
using namespace cv;

int OpenCvCamera::capture()
{
    VideoCapture cam;
    namedWindow("video");
    
    //像素格式转换上下文
    SwsContext *swsContext = NULL;
    
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
        
        
        Mat frame;
        for(;;)
        {
            cam.read(frame);
            imshow("video", frame);
            waitKey(1);
        }
    } catch(exception &ex)
    {
        if(cam.isOpened()){
            cam.release();
        }
        if(swsContext)
        {
            sws_freeContext(swsContext);
            swsContext = NULL;
        }
         cerr << ex.what() <<endl;
    }
    
    getchar();
    return 0;
}
