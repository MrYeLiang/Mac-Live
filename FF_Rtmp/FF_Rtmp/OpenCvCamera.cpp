//
//  OpenCvCamera.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/5/22.
//  Copyright © 2019 叶亮. All rights reserved.
//

#include "OpenCvCamera.hpp"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

#pragma comment(lib,"opencv_world320lib“)

using namespace std;
using namespace cv;

int OpenCvCamera::capture()
{
    VideoCapture cam;
    namedWindow("video");
    
    try
    {
        if(cam.open(0)){
            cout << "open camera success!" << endl;
        }else{
            cout << "open camera failed!" << endl;
            
            waitKey(1);
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
         cerr << ex.what() <<endl;
    }
    
    getchar();
    return 0;
}
