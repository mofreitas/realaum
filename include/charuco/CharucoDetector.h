#ifndef CHARUCO_DETECTOR_CPP
#define CHARUCO_DETECTOR_CPP

#include <opencv2/opencv.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

class CharucoDetector{
private:
    cv::Ptr<cv::aruco::Dictionary> dictionary;
    cv::Ptr<cv::aruco::CharucoBoard> board;
    cv::Mat cameraMatrix, distCoeffs;
    glm::mat4 cameraExtrinsicMatrixGL;
    glm::mat4 GL2CVMatrix;

    bool readCameraParameters(std::string filename, cv::Mat &camMatrix, cv::Mat &distCoeffs) {
        cv::FileStorage fs(filename, cv::FileStorage::READ);
        if(!fs.isOpened())
            return false;
        fs["camera_matrix"] >> camMatrix;
        fs["distortion_coefficients"] >> distCoeffs;
        return true;
    }

    //https://answers.opencv.org/question/23089/opencv-opengl-proper-camera-pose-using-solvepnp/
    glm::mat4 cvVec2glmMat(cv::Vec3d rot_vec, cv::Vec3d trans_vec){
        cv::Mat mr;
        cv::Rodrigues(rot_vec, mr);
        glm::mat4 output = glm::mat4(
                         mr.at<double>(0, 0), -mr.at<double>(1, 0), -mr.at<double>(2, 0), 0, 
                         mr.at<double>(0, 1), -mr.at<double>(1, 1), -mr.at<double>(2, 1), 0,
                         mr.at<double>(0, 2), -mr.at<double>(1, 2), -mr.at<double>(2, 2), 0,
                         trans_vec[0]       , -trans_vec[1]       , -trans_vec[2]       , 1);          
        //std::cout << "pre-translation: " << output[3][0] << ", " << output[3][1] << ", " << output[3][2] << std::endl;
        output = glm::rotate(output, glm::radians(90.0f), glm::vec3(1.0,0.0, 0.0));
        //std::cout << "translation: " << output[3][0] << ", " << output[3][1] << ", " << output[3][1] << std::endl;
        return output;
    }

public:
    CharucoDetector(std::string calibration_filename, double far=1000, double near=0.1){
        this->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
        this->board = cv::aruco::CharucoBoard::create(5, 7, 0.07, 0.05, dictionary);
        // camera parameters are read from somewhere

        //this->GL2CVMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0,0.0, 0.0));

        //TODO implementar calibração
        assert(!calibration_filename.empty());
        readCameraParameters(calibration_filename, this->cameraMatrix, this->distCoeffs);   
    }

    cv::Mat getCameraMatrix(){
        return this->cameraMatrix;
    }

    glm::mat4 get_charuco_extrinsics(cv::Mat image){
        cv::Mat imageCopy;
        cv::Vec6d output;
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
                //cv::aruco::drawDetectedCornersCharuco(imageCopy, charucoCorners, charucoIds, cv::Scalar(255, 0, 0));
                cv::Vec3d rvec, tvec;
                bool valid = cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, board, cameraMatrix, distCoeffs, rvec, tvec);
                // if charuco pose is valid
                if(valid){
                    cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, 0.1, 1);
                    
                    this->cameraExtrinsicMatrixGL = cvVec2glmMat(rvec, tvec);
                }

            }
        }
        return this->cameraExtrinsicMatrixGL;
    }    
};
#endif