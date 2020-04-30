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
unsigned char *cvMat2TexInput(Mat &img); 
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
    Model ourModel("./resources/objects/column/column.obj");

    //Obtendo matrix de projeção
    cd.resizeCameraMatrix(c.getFrameWidth(), c.getFrameHeight());
    glm::mat4 projection = getProjetionMatrix(
        cd.getCameraMatrix(), c.getHalfScreenWidth(),
        c.getHalfScreenHeight(), near, far);

    //Inicializando/Aplicando distorção
    double coeff_dist[4] = {1, 2, 0, 1.0};
    Mat pontos_dist, coefficients(1, 4, CV_64FC1, coeff_dist);    
    pontos_dist = initializeBarrelDistortionParameters(
        c.getFrameWidth(), c.getFrameHeight(), cd.getCameraMatrix(),
        coefficients);

    // Auxiliary variables
    unsigned char* image_data;
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
        if (image_data){
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, c.getHalfScreenWidth(), c.getHalfScreenHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
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

        glm::mat4 model = glm::scale(eye, glm::vec3(0.0001f, 0.0001f, 0.0001f)); 
       
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", viewMatrix);
        ourShader.setMat4("model", model);
        ourShader.setVec3("viewPos", glm::vec3(viewMatrix[0][3], viewMatrix[1][3], viewMatrix[2][3]));
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
        if(waitKey(1) == 27) break;         

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

unsigned char *cvMat2TexInput(Mat &img)
{
    Mat image;
    cvtColor(img, image, COLOR_BGR2RGB);
    flip(image, image, 0);
    return image.data;
}

GLFWwindow* initializeProgram(GLuint width, GLuint height){
    // glfw: initialize and configure
    // ------------------------------
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
            cout << "Usage: ./oficial.out -d -c <path to camera parameters> -pi <user@ip>" << endl;
            exit(-1);
        }
    }

    for(int i = 0; i < n_obg; i++){
        if(strcmp(obg[i], "\0")){
            cout << "Parameter " << obg[i] << " is required" << endl;
            exit(0);
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
        1.0 - 2.0*cameraMatrix.at<double>(0, 2)/halfScreenWidth, -1.0 + (2.0*cameraMatrix.at<double>(1, 2))/halfScreenHeight, (near+far)/(near-far), -1,  
        0, 0, 2*near*far/(near-far), 0
    );
}
