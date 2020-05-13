#include <Comunication.h>
#include <charuco/CharucoDetector.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <iostream>
#include <chrono>

#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"

using namespace cv;

GLFWwindow* initializeProgram(GLuint width, GLuint height);
void processArgs(int argv, char** argc);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void applyFishEyeDistortion(Mat& img, Mat cameraMatrix, Mat coefficients);
Mat cvMat2TexInput(Mat &img); 
Mat initializeBarrelDistortionParameters(int width, int height, Mat cameraMatrix, Mat distCoefficients);
glm::mat4 getProjetionMatrix(Mat cameraMatrix, int halfScreenWidth, int halfScreenHeight, double near, double far);

// settings
bool debugMode = false;
char* pi_credentials;
string pathToCameraParameters;
double far=100, near=0.1;

float vertices[] = {
    //     Position       TexCoord
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // below left
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f   // below right
};

unsigned int indices[] = {
    0, 1, 2,
    1, 2, 3
};

int main(int argc, char** argv)
{   
    processArgs(argc, argv);

    //Read camera intrisics
    CharucoDetector cd(pathToCameraParameters, debugMode);

    //Create/Open/Start Communication
    Comunication c(40001, debugMode, cd.getCameraWidth(), cd.getCameraHeight());

    try{
        c.openComunicators(
            "appsrc is-live=true ! queue ! videoscale ! video/x-raw, width={width}, height={height} ! videoconvert ! video/x-raw, format=I420 ! queue ! vaapih264enc quality-level=7 keyframe-period=20 ! video/x-h264, stream-format=avc, profile=baseline ! queue ! rtph264pay config-interval=1 ! udpsink host={ip} port=5000 sync=false",
            pi_credentials == NULL ? string() : pi_credentials,
            "udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96, a-framerate=20 ! rtpjitterbuffer drop-on-latency=true ! rtph264depay ! queue ! decodebin ! videoconvert ! queue ! appsink"
        );
    }
    catch(...){
        return 0;
    }
    
    c.startComunication();
    
    GLFWwindow* window = initializeProgram((GLuint)c.getHalfScreenWidth(), (GLuint)c.getHalfScreenHeight());  

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int aTexture;
    glGenTextures(1, &aTexture);
    glBindTexture(GL_TEXTURE_2D, aTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Build and compile shadders
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs"); 
    Shader background_shader("./background_shader.vs", "./background_shader.fs");

    // Load model
    Model ourModel("./resources/objects/ncc1701/ncc1701.obj");

    //Obtendo matrix de projeção
    cd.resizeCameraMatrix(c.getFrameWidth(), c.getFrameHeight());
    glm::mat4 projection = getProjetionMatrix(
        cd.getCameraMatrix(), c.getHalfScreenWidth(),
        c.getHalfScreenHeight(), near, far);

    glm::vec3 scale(1.0, 1.0, 1.0), desl(0.0, 0.0, 0.0);

    //Inicializando/Aplicando distorção
    double coeff_dist[4] = {1, 2, 0, 1.0};
    Mat pontos_dist, coefficients(1, 4, CV_64FC1, coeff_dist);    
    pontos_dist = initializeBarrelDistortionParameters(
        c.getFrameWidth(), c.getFrameHeight(), cd.getCameraMatrix(),
        coefficients);

    // Auxiliary variables
    Mat image_data;
    Mat image_read, image_write(c.getHalfScreenHeight(), c.getHalfScreenWidth(), CV_8UC3);
    Mat image_barrel, output(c.getScreenHeight(), c.getScreenWidth(), CV_8UC3);    
    glm::mat4 eye(1.0f);
    
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Process keys inputs
        processInput(window);

        // Render background
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Drawing background       
        background_shader.use();
        glActiveTexture(GL_TEXTURE0);
        background_shader.setInt("aTexture", 0);
        glBindTexture(GL_TEXTURE_2D, aTexture);        

        image_read = c.readImage();

        //getting viewing matrix from charuco
        glm::mat4 viewMatrix = cd.getViewMatrix(image_read);

        image_data = cvMat2TexInput(image_read);  
        if (!image_data.empty()){
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, c.getHalfScreenWidth(), c.getHalfScreenHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.data);
        }
        else{
            std::cout << "Failed to load texture: Empty image" << std::endl;
            exit(1);
        }
        
        //Desenha plano de fundo
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //RESETA BIND
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);

        glm::mat4 model = glm::translate(eye, desl);
        model = glm::scale(model, scale); 
       
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", viewMatrix);
        ourShader.setMat4("model", model);
        ourShader.setVec3("viewPos", 0, 0, 0);
        ourModel.Draw(ourShader);

        glDisable(GL_DEPTH_TEST);
 
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, image_read.cols, image_read.rows, GL_BGR, GL_UNSIGNED_BYTE, image_write.data);

        flip(image_write, image_write, 0);
        
        //Applying barrel distortion        
        remap(image_write, image_write, pontos_dist, noArray(), cv::INTER_LINEAR);

        image_write.copyTo(output(Rect(0, 0, c.getHalfScreenWidth(), c.getHalfScreenHeight())));
	    image_write.copyTo(output(Rect(c.getHalfScreenWidth(), 0, c.getHalfScreenWidth(), c.getHalfScreenHeight())));        
        c.writeImage(output);

        imshow("saida", output);
        char key = waitKey(1);
        switch(key){
            case 27:
                glfwSetWindowShouldClose(window, true);
                break;
            case 'w':
                desl[1] = desl[1] + scale[0];
                break;
            case 's':
                desl[1] = desl[1] - scale[0];
                break;
            case 'd':
                desl[0] = desl[0] + scale[0];
                break;
            case 'a':
                desl[0] = desl[0] - scale[0];
                break;
            case 'q':
                desl[2] = desl[2] + scale[0];
                break;
            case 'e':
                desl[2] = desl[2] - scale[0];
                break;
            case 'i':
                scale = scale * 2.0f;
                cout << "Scale: " << scale[0] << endl;
                break;
            case 'o':
                scale = scale * 0.5f;
                cout << "Scale: " << scale[0] << endl;
                break;
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

Mat cvMat2TexInput(Mat &img)
{
    Mat image;
    cvtColor(img, image, COLOR_BGR2RGB);
    flip(image, image, 0);
    return image;
}

GLFWwindow* initializeProgram(GLuint width, GLuint height){
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, 0);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif   

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(width, height, "Realaum", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    return window;
}

void processArgs(int argc, char** argv){
    int n_obg = 2;
    char obg[][10] = {"-pi\0", "-c\0"};
    //Informa se é obrigatório mesmo com o modo debug ativado
    bool mode_d[] = {false, true};

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-d") == 0){
            cout << "Modo debug ativo" << endl;
            debugMode = true;
        }  
        else if(strcmp(argv[i], "-pi")==0 && argc > i+1){
            pi_credentials = argv[i+1];
            memset(obg[0], '\0', 10);;
            i++;
        }
        else if(strcmp(argv[i], "-c")==0 && argc > i+1){
            pathToCameraParameters = string(argv[i+1]);
            memset(obg[1], '\0', 10);;
            i+=1;
        }
        else{
            cout << "Comando " << argv[i] << "inválido" << endl;
            cout << "Usage: ./oficial.out -d -c <path to camera parameters> -pi <user@ip>" << endl
                 << "Options:" << endl 
                 << "-d : [Opcional] Modo debug" << endl 
                 << "-c : [Obrigatorio] Informa o caminhos dos da calibracao da camera" << endl
                 << "-pi: [Obrigatorio se -d não for ativado] informa o usuário@ip do raspberry" << endl;
            exit(-1);
        }
    }

    for(int i = 0; i < n_obg; i++){
        if(strcmp(obg[i], "\0") && (mode_d[i] && debugMode)){
            cout << "Parameter " << obg[i] << " is required" << endl;
            exit(1);
        }
    }
}

/** Inicializa matrix utilizada para distorcer imagem 
 */
Mat initializeBarrelDistortionParameters(int width, int height, Mat cameraMatrix, Mat distCoefficients){
    assert(distCoefficients.cols == 4 && distCoefficients.rows == 1);

    Mat pontos(height, width, CV_32FC2), pontos_dist = Mat(height, width, CV_32FC2);
    Matx33d camMat = cameraMatrix; //K is my camera matrix estimated by cv::fisheye::calibrate
    Matx33d camMat_inv = camMat.inv(); //Inverting the camera matrix

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            Vec3f v(j, i, 1.0);
            Vec3f r = camMat_inv * v;
            pontos.at<Vec2f>(i, j)[0] = r[0];
            pontos.at<Vec2f>(i, j)[1] = r[1];
        }
    }

    fisheye::distortPoints(pontos, pontos_dist, camMat, distCoefficients);
    return pontos_dist;
}

void applyBarrelDistortion(Mat& img, Mat& pontos_dist){    
    remap(img, img, pontos_dist, noArray(), cv::INTER_LINEAR);
}

glm::mat4 getProjetionMatrix(Mat cameraMatrix, int halfScreenWidth, int halfScreenHeight, double near, double far){
    return glm::mat4(
        2.0*cameraMatrix.at<double>(0, 0)/halfScreenWidth, 0, 0, 0,
        0, 2.0*cameraMatrix.at<double>(1, 1)/halfScreenHeight, 0, 0,
        -(1.0 - 2.0*cameraMatrix.at<double>(0, 2)/halfScreenWidth), -(-1.0 + (2.0*cameraMatrix.at<double>(1, 2))/halfScreenHeight), (-near-far)/(near-far), 1,  
        0, 0, 2*near*far/(near-far), 0
    );
}



















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

        //Inverte y e z, uma vez que esse eixos são invertidos em opengl
         glm::mat4 output = glm::mat4(
                         mr(0, 0)    , -mr(1, 0)     , mr(2, 0)    , 0, 
                         mr(0, 1)    , -mr(1, 1)     , mr(2, 1)    , 0,
                         mr(0, 2)    , -mr(1, 2)     , mr(2, 2)    , 0,
                         trans_vec[0], -trans_vec[1], trans_vec[2], 1);  

        //Objets são desenhados tendo em mente o eixo y como topo, portanto rotaciona-o
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