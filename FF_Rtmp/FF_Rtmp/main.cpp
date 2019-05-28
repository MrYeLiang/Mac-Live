//
//  main.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/4/29.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "TestRtmpPusher.hpp"
#include "OpenCvCamera.hpp"

int main(int argc, char *argv[])
{
    //01 rtmp推流(视频源为本地)测试
//    TestRtmpPusher pusher;
//    pusher.doPush();
    
    //02 视频采集并推流
    OpenCvCamera camera;
    camera.capture();
}
    

