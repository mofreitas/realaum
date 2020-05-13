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
    int cameraWidth, cameraHeight;
    bool debugMode;

    void readCameraParameters(std::string filename) {
        cv::FileStorage fs(filename, cv::FileStorage::READ);
        if(!fs.isOpened())
            throw 1;
        fs["camera_matrix"] >> this->cameraMatrix;
        fs["distortion_coefficients"] >> this->distCoeffs;
        fs["image_width"] >> cameraWidth;
        fs["image_height"] >> cameraHeight;
    }

    //https://answers.opencv.org/question/23089/opencv-opengl-proper-camera-pose-using-solvepnp/
    glm::mat4 cvVec2glmMat(cv::Vec3d rot_vec, cv::Vec3d trans_vec){
        cv::Matx33d mr;
        cv::Rodrigues(rot_vec, mr);
        
        //Inverte eixo y, pois é invertido em opencv (eixo y aponta para debaixo da camera)
        glm::mat4 output = glm::mat4(
                         mr(0, 0)    , -mr(1, 0)    , mr(2, 0)    , 0, 
                         mr(0, 1)    , -mr(1, 1)    , mr(2, 1)    , 0,
                         mr(0, 2)    , -mr(1, 2)    , mr(2, 2)    , 0,
                         trans_vec[0], -trans_vec[1], trans_vec[2], 1);          
        
        //output = glm::rotate(output, glm::radians(90.0f), glm::vec3(1.0,0.0, 0.0));
        return output;
    }

public:
    CharucoDetector(std::string calibration_filename, bool debugMode=false){
        this->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
        this->board = cv::aruco::CharucoBoard::create(5, 7, 0.07, 0.05, dictionary);
        this->debugMode = debugMode;
        // camera parameters are read from somewhere

        //TODO implementar calibração
        assert(!calibration_filename.empty());
        readCameraParameters(calibration_filename);   
    }

    int getCameraWidth(){
        return this->cameraWidth;
    }

    int getCameraHeight(){
        return this->cameraHeight;
    }

    cv::Mat getCameraMatrix(){
        return this->cameraMatrix;
    }

    void resizeCameraMatrix(int newWidth, int newHeight, bool cropped = true){
        assert(newWidth != 0 && newHeight != 0);

        if(cropped){
            this->cameraMatrix.at<double>(0, 2) += (double) (newWidth-this->cameraWidth)*0.5;
            this->cameraMatrix.at<double>(1, 2) += (double) (newHeight-this->cameraHeight)*0.5;
        }
        else{
            this->cameraMatrix.at<double>(0, 0) = (double) this->cameraMatrix.at<double>(0, 0)*newWidth/this->cameraWidth;
            this->cameraMatrix.at<double>(1, 1) = (double) this->cameraMatrix.at<double>(1, 1)*newHeight/this->cameraHeight;
            this->cameraMatrix.at<double>(0, 2) = (double) this->cameraMatrix.at<double>(0, 2)*newWidth/this->cameraWidth;
            this->cameraMatrix.at<double>(1, 2) = (double) this->cameraMatrix.at<double>(1, 2)*newHeight/this->cameraHeight;
        }
    }

    cv::Mat getDistCoeffs(){
        return this->distCoeffs;
    }

    cv::Mat getDistCoeffs4(){
        return this->distCoeffs(cv::Rect(0, 0, 4, 1));
    }

    glm::mat4 getViewMatrix(cv::Mat image){
        cv::Mat imageCopy;
        cv::Vec6d output;
        //image.copyTo(imageCopy);
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
                    if(debugMode)
                        cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, 0.1, 1);
                    
                    this->cameraExtrinsicMatrixGL = cvVec2glmMat(rvec, tvec);
                }

            }
        }
        return this->cameraExtrinsicMatrixGL;
    }    
};
#endif