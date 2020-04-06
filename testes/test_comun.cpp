#include "Comunication.cpp"

#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"

using namespace cv;
using namespace std;

int main(){
    Comunication c("192.168.0.14", 40001, false);
    //"udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96, a-framerate=20 ! rtpjitterbuffer drop-on-latency=true ! rtph264depay ! queue ! decodebin ! videoconvert ! video/x-raw, format=RGBx ! queue ! appsink",


    c.openComunicators(
        "v4l2src device=/dev/video0 ! videoconvert ! video/x-raw, width=640 ! queue ! appsink",
        "appsrc is-live=true ! queue ! videoconvert ! video/x-raw, format=I420 ! queue ! vaapih264enc rate-control=2 ! video/x-h264, profile=high, stream-format=byte-stream ! queue ! rtph264pay config-interval=1 ! udpsink host=192.168.0.14 port=5000 sync=false"
    );
    c.startComunication();

    Mat image; 
    while(1<10){
        image = c.readImage();  
        //image.convertTo(image, RGB);       
        cout << (int) image.at<Vec3b>(50, 50)[0] << ", "
             << (int) image.at<Vec3b>(50, 50)[1] << ", "
             << (int) image.at<Vec3b>(50, 50)[2] << " - " 
             << image.channels() << ", " << image.type() << image.depth() << endl; 

        imshow("saida", image);
        if(waitKey(20)==27) break;
    }
    return 0;
}