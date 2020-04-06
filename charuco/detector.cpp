#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <vector>
#include <iostream>
#include <ctime>

using namespace std;
using namespace cv;

static bool readCameraParameters(string filename, Mat &camMatrix, Mat &distCoeffs) {
    FileStorage fs(filename, FileStorage::READ);
    if(!fs.isOpened())
        return false;
    fs["camera_matrix"] >> camMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    return true;
}

int main(){
    cv::VideoCapture inputVideo;
    inputVideo.open(0);
        inputVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        inputVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cv::Mat cameraMatrix, distCoeffs;
    // camera parameters are read from somewhere
    readCameraParameters("./saida.txt", cameraMatrix, distCoeffs);
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
    cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(5, 7, 0.05, 0.03, dictionary);
    while (inputVideo.grab()) {
        cv::Mat image, imageCopy;
        inputVideo.retrieve(image);
        image.copyTo(imageCopy);
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::aruco::detectMarkers(image, dictionary, corners, ids);
        // if at least one marker detected
        if (ids.size() > 0) {
            std::vector<cv::Point2f> charucoCorners;
            std::vector<int> charucoIds;
            cv::aruco::interpolateCornersCharuco(corners, ids, image, board, charucoCorners, charucoIds, cameraMatrix, distCoeffs);
            // if at least one charuco corner detected
            if(charucoIds.size() > 0) {
                cv::aruco::drawDetectedCornersCharuco(imageCopy, charucoCorners, charucoIds, cv::Scalar(255, 0, 0));
                cv::Vec3d rvec, tvec;
                bool valid = cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, board, cameraMatrix, distCoeffs, rvec, tvec);
                // if charuco pose is valid
                if(valid){
                    cv::aruco::drawAxis(imageCopy, cameraMatrix, distCoeffs, rvec, tvec, 0.1);
                    cout << "rvec: "<< rvec[0] << ", " << rvec[1] << ", " << rvec[2] << endl;
                    cout << "tvec: "<< tvec[0] << ", " << tvec[1] << ", " << tvec[2] << endl;
                }

            }
        }
        cv::imshow("out", imageCopy);
        char key = (char) cv::waitKey(30);
        if (key == 27)
            break;
    }
}