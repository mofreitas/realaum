#include <opencv2/aruco/charuco.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/calib3d.hpp>

using namespace cv;

int main(){

    calibrateCamera()

    Ptr<aruco::DetectorParameters> detectorParams = aruco::DetectorParameters::create();

    cv::VideoCapture inputVideo;
    inputVideo.open(0);
    cv::Mat cameraMatrix, distCoeffs;
    // camera parameters are read from somewhere
    readCameraParameters(cameraMatrix, distCoeffs);
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(5, 7, 0.04, 0.02, dictionary);
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
                if(valid)
                    cv::aruco::drawAxis(imageCopy, cameraMatrix, distCoeffs, rvec, tvec, 0.1);
            }
        }
        cv::imshow("out", imageCopy);
        char key = (char) cv::waitKey(waitTime);
        if (key == 27)
            break;
    }
}