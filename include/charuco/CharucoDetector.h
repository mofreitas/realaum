#ifndef CHARUCO_DETECTOR_CPP
#define CHARUCO_DETECTOR_CPP

#include <vector>
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
    cv::Size_<float> charucoBoardObjSize;

    glm::mat4 cameraExtrinsicMatrixGL;
    glm::mat4 GL2CVMatrix;

    std::vector<cv::Point3f> objectPoints;
    std::vector<cv::Point3f> objectPoints_centralized;

    int cameraWidth, cameraHeight;
    bool debugMode;

    void readCameraParameters(std::string filename) {
        cv::FileStorage fs(filename, cv::FileStorage::READ);
        if(!fs.isOpened()){
            std::cout << "Cannot open " << filename << std::endl;
            throw 1;
        }
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
        return output;
    }
    
    /**https://answers.opencv.org/question/179182/center-of-charuco-board/*/
    void getCenterCharucoBoard(cv::Mat& image, cv::Vec3d& rvec, cv::Vec3d& tvec, cv::Vec3d& rvec_c, cv::Vec3d& tvec_c){
        std::vector<cv::Point2f> imagePoints;
        
        //Projeta os pontos 3d em coordenadas de objeto no plano 2d baseados no rvec e tvec 
        //Neste caso, verifica qual a projeção 2d dos quatro cantos do tabuleiro          
        cv::projectPoints(this->objectPoints, rvec, tvec, this->cameraMatrix, this->distCoeffs, imagePoints);       
        
        //O frame axis é sempre desenhado no origem (rvec e tvec), por isso, posiciona  objeto de forma
        //que seu meio fique no centro        
        cv::solvePnP(this->objectPoints_centralized, imagePoints, this->cameraMatrix, this->distCoeffs, rvec_c, tvec_c);
        
        //Desenha círculos na ponta de tabuleiro e o eixo e coordenadas
        if(debugMode){
            cv::circle(image, imagePoints[0], 10, cv::Scalar(0,0,255), 3);
            cv::circle(image, imagePoints[1], 10, cv::Scalar(0,255,0), 3);
            cv::circle(image, imagePoints[2], 10, cv::Scalar(255,0,0), 3);
            cv::circle(image, imagePoints[3], 10, cv::Scalar(0,255,255), 3);
            cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec_c, tvec_c, 0.1, 3);
        }        
    }

    void getRvecTvecFromViewMatrix(const glm::mat4& viewMatrix, cv::Vec3f& rvec, cv::Vec3f& tvec){
        tvec = cv::Vec3f(0, 0, 0);
        tvec[0] = viewMatrix[3][0];
        tvec[1] = -viewMatrix[3][1];
        tvec[2] = viewMatrix[3][2];

        cv::Mat rmat(3, 3, CV_64FC1);
        rmat.at<float>(0, 0) = viewMatrix[0][0];
        rmat.at<float>(0, 1) = viewMatrix[1][0];
        rmat.at<float>(0, 2) = viewMatrix[2][0];
        rmat.at<float>(1, 0) = -viewMatrix[0][1];
        rmat.at<float>(1, 1) = -viewMatrix[1][1];
        rmat.at<float>(1, 2) = -viewMatrix[2][1];
        rmat.at<float>(2, 0) = viewMatrix[0][2];
        rmat.at<float>(2, 1) = viewMatrix[1][2];
        rmat.at<float>(2, 2) = viewMatrix[2][2];
        cv::Rodrigues(rvec, rmat);
    }

public:
    CharucoDetector(std::string calibration_filename, bool debugMode=false){
        this->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
        this->board = cv::aruco::CharucoBoard::create(5, 7, 0.07, 0.05, dictionary);
        this->debugMode = debugMode;
        this->cameraExtrinsicMatrixGL = glm::mat4(0.0f);        
        
        int squares_w = this->board->getChessboardSize().width;
        int squares_h = this->board->getChessboardSize().height;
        float sq_size = this->board->getSquareLength();
        
        //Uma vez que o canto inferior esquerdo está no ponto (0, 0, 0), insere localizações dos outros
        //cantos
        this->objectPoints.push_back(cv::Point3f(0                , sq_size*squares_h, 0));
        this->objectPoints.push_back(cv::Point3f(sq_size*squares_w, sq_size*squares_h, 0));
        this->objectPoints.push_back(cv::Point3f(sq_size*squares_w, 0                , 0));
        this->objectPoints.push_back(cv::Point3f(0                , 0                , 0));  
        
        //Esta ordem de inserção é exigida na documentação, de modo que o tabuleiro fique no centro
        //do sistema de coordenadas        
        this->objectPoints_centralized.push_back(cv::Point3f(-sq_size*squares_w/2.0,  sq_size*squares_h/2.0, 0));
        this->objectPoints_centralized.push_back(cv::Point3f( sq_size*squares_w/2.0,  sq_size*squares_h/2.0, 0));
        this->objectPoints_centralized.push_back(cv::Point3f( sq_size*squares_w/2.0, -sq_size*squares_h/2.0, 0));
        this->objectPoints_centralized.push_back(cv::Point3f(-sq_size*squares_w/2.0, -sq_size*squares_h/2.0, 0));

        /*TODO implementar calibração*/
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
    
    float getSquareLength(){
        return this->board->getSquareLength();
    }

    cv::Size getChessboardSize(){
        return this->board->getChessboardSize();
    }

    glm::vec3 getAutoScaleVector(const glm::vec3 size_object, const cv::Mat image, char axis = 'x'){
        
        int squares_w = this->board->getChessboardSize().width;
        int squares_h = this->board->getChessboardSize().height;
        float sq_size = this->board->getSquareLength();

        glm::vec3 scale(1.0f);
        glm::mat4 viewMatrix(0.0f);
        cv::Vec3f p1, p2;
        std::vector<cv::Point2f> imagePoints;
        std::vector<cv::Point3f> objectPoints_topright;
        objectPoints_topright.reserve(4);

        objectPoints_topright.push_back(cv::Point3f(-sq_size*squares_w, 0                 , 0));
        objectPoints_topright.push_back(cv::Point3f(0                 , 0                 , 0));
        objectPoints_topright.push_back(cv::Point3f(0                 , -sq_size*squares_h, 0));
        objectPoints_topright.push_back(cv::Point3f(-sq_size*squares_w, -sq_size*squares_h, 0));

        viewMatrix = getViewMatrix(image);
        if (viewMatrix[0][0] == 0.0f)
            return glm::vec3(0.0f, 0.0f,0.0f);

        cv::Vec3f rvec, tvec;
        std::cout << viewMatrix[0][0] << std::endl;
        getRvecTvecFromViewMatrix(viewMatrix, rvec, tvec);

        cv::projectPoints(this->objectPoints, rvec, tvec, this->cameraMatrix, this->distCoeffs, imagePoints);
        cv::solvePnP(this->objectPoints, imagePoints, this->cameraMatrix, this->distCoeffs, rvec, p1);
        cv::solvePnP(objectPoints_topright, imagePoints, this->cameraMatrix, this->distCoeffs, rvec, p2);

        double size_x = cv::norm(p1, p2, cv::NORM_L2, cv::Vec3b(255, 0, 0));
        double size_y = cv::norm(p1, p2, cv::NORM_L2, cv::Vec3b(0, 255, 0));

        if (axis == 'x'){
            scale = scale * ((float)size_x / size_object[0]);
        }
        else if (axis == 'y'){
            scale = scale * ((float)size_y / size_object[1]);
        }
        else if (axis == 'z'){
            scale = scale * ((float)tvec[2] / size_object[2]);
        }

        this->charucoBoardObjSize = cv::Size_<float>((float)size_x, (float)size_y);

        //Condição feita apenas no final para que charucoBoardObjSize possa ser preenchido
        if(axis != 'x' && axis != 'y' && axis != 'z')
            return glm::vec3(1.0f, 1.0f, 1.0f);
        return scale;
    }    

    /**
        O modo cropped = false apenas funcionas com imagens redimensionadas por meio de software
    */
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

    cv::Size_<float> getCharucoBoardObjSize(){
        return this->charucoBoardObjSize;
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
                //Corner vectors
                cv::Vec3d rvec, tvec;
                //Center vectors
                cv::Vec3d rvec_c, tvec_c;
                bool valid = cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, board, cameraMatrix, distCoeffs, rvec, tvec);
                // if charuco pose is valid
                if(valid){
<<<<<<< HEAD
                    //if(debugMode)
                    //    cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, 0.1, 1);
=======
                    if(debugMode)
-                       cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, 0.1, 1);
>>>>>>> master
                    
                    getCenterCharucoBoard(image, rvec, tvec, rvec_c, tvec_c);
                    this->cameraExtrinsicMatrixGL = cvVec2glmMat(rvec_c, tvec_c);
                }

            }
        }
        return this->cameraExtrinsicMatrixGL;
    }    
};
#endif
