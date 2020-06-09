#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <string>
#include <chrono>
#include <stdio.h>

using namespace cv;
using namespace std;

int main(){
    float fps;

    VideoCapture cap;
    cap.open(1);
    if (!cap.isOpened())
    {
        std::cout << "Camera not opened!" << std::endl;
        return -1;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,240);

    Mat frame;
    int frames = 0;
    chrono::time_point<std::chrono::steady_clock> begin = chrono::steady_clock::now();
    char array[50] = {'0'};
    
    for(;;){
        
        cap >> frame;
        putText(frame, array, Point(30,30), 
            FONT_HERSHEY_COMPLEX_SMALL, 0.8, Scalar(200,200,250), 1);
        
        chrono::time_point<std::chrono::steady_clock> end = chrono::steady_clock::now();
            
        imshow("windows", frame);        
        
        frames++;
        if(waitKey(1)==27) break;
        sprintf (array, "%f [ms]", (frames*1000.0)/chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        
    }
}
