#include <Comunication.h>
#include <charuco/CharucoDetector.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <stdio.h>
#include <iostream>
#include <chrono>

#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"

using namespace cv;

/**
 * Efetua a inicialização do OPENGL
 * 
 * @param width Largura da janela.
 * @param height Altura da janela.
 */
GLFWwindow* initializeProgram(GLuint width, GLuint height);

/**
 * Processa argumentos passados no momento de execução do programa
 * 
 * @param argc Número de parâmetros
 * @param argv Parâmetros
 */
void processArgs(int argv, char** argc);

/**
 * Efetua as operações no objeto baseada na entrda do usuário
 * 
 * @param key Tecla pressionada
 * @param step Salto do movimento de translação, função da escala do objeto
 * @param model Matriz de model da tranformação de coordenadas
 */
void processInputCV(char key, float& step, glm::mat4& model);

/**
 * Converte uma matrix OPENCV para dados OPENGL
 * 
 * @param img Imagem que será convertida para OPENGL
 * 
 * @return Imagem convertida para OPENGL
 */
Mat cvMat2TexInput(Mat &img); 

/** 
 * Inicializa matrix utilizada para distorcer imagem, mapeando os pontos de uma imagem
 * normal em outra distorcida
 * 
 * @param width Largura da imagem a distorcer
 * @param height Altura da imagem a distorcer
 * @param cameraMatrix Matriz da câmera da imagem a distorcer
 * @param distCoefficients Coeficientes de distorção desejado na imagem de saída
 * 
 * @return Matriz que mapeia a disposição dos pontos da imagem a ser distorcida
 */
Mat initializeBarrelDistortionParameters(int width, int height, Mat cameraMatrix, Mat distCoefficients);

/**
 * Gera a matrix de projeção da trasnformação de coordenadas
 * 
 * @param cameraMatrix Matriz intríseca da camera
 * @param screenWidth Largura da tela
 * @param screenHeight Altura da tela 
 * @param near A distância do plano near
 * @param far A distância do plano far
 */
glm::mat4 getProjetionMatrix(Mat cameraMatrix, int screenWidth, int screenHeight, double near, double far);

/**
 * Aplica a distorção utilizando o mapeamento gerado na função initializeBarrelDistortionParameters
 * 
 * @param img Image a ser distorcida
 * @param pontos_dist Matriz informando nova posição dos pontos da imagem 
 */
void applyBarrelDistortion(Mat& img, Mat& pontos_dist);

// settings
bool debugMode = false;
char* pi_credentials;
string pathToCameraParameters;
double far=100, near=0.1;
glm::mat4 scale_matrix = glm::mat4(1.0f);
string modelPath;
char autoScaleAxis = 'n';
string iphost = "\0";
int cameraIndex = 0;

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
    Comunication c(iphost, 40001, debugMode, cd.getCameraWidth(), cd.getCameraHeight());

    try{
        c.openComunicators(
            cameraIndex,
            "appsrc is-live=true ! queue ! videoscale ! video/x-raw, width={width}, height={height} ! videoconvert ! video/x-raw, format=I420 ! queue ! vaapih264enc quality-level=7 keyframe-period=20 ! video/x-h264, stream-format=avc, profile=baseline ! queue ! rtph264pay config-interval=1 ! udpsink host={ip} port=5000",
            pi_credentials == NULL ? string() : pi_credentials,
            "udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96, a-framerate=20 ! rtpjitterbuffer drop-on-latency=true latency=100 ! rtph264depay ! queue ! decodebin ! videoconvert ! queue ! appsink"
        );
    }
    catch(...){
        return 0;
    }
    
    c.startComunication();
    
    GLFWwindow* window = initializeProgram((GLuint)c.getHalfScreenWidth(), (GLuint)c.getHalfScreenHeight());  

    //Gera polígono em que será aplicado o background (imagem capturada pela câmera)
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
    Model ourModel(modelPath);    

    //Obtendo matrix de projeção
    cd.resizeCameraMatrix(c.getFrameWidth(), c.getFrameHeight());
    glm::mat4 projection = getProjetionMatrix(
        cd.getCameraMatrix(), c.getHalfScreenWidth(),
        c.getHalfScreenHeight(), near, far);

    //Inicializando/Aplicando distorção
    double coeff_dist[4] = {1, 2, 0, 0};
    Mat pontos_dist, coefficients(1, 4, CV_64FC1, coeff_dist);    
    pontos_dist = initializeBarrelDistortionParameters(
        c.getFrameWidth(), c.getFrameHeight(), cd.getCameraMatrix(),
        coefficients);

    // Auxiliary variables
    Mat image_data;
    Mat image_read, image_write(c.getHalfScreenHeight(), c.getHalfScreenWidth(), CV_8UC3);
    Mat image_barrel, output(c.getScreenHeight(), c.getScreenWidth(), CV_8UC3); 

    float step = 0.0;
    glm::vec3 scale(0.0f);
    glm::mat4 model(1.0f);
    glm::vec3 trans(1.0f);
    glm::vec4 lightPos(0.0f, 0.0f, 50.0f, 1.0f);
    
    int fps=0;
    unsigned long int frames = 0;
    long int frames_times[5] = {0};
    char fps_text[50] = {'0'};
    chrono::time_point<std::chrono::steady_clock> begin = chrono::steady_clock::now();
    chrono::time_point<std::chrono::steady_clock> before = begin;
    chrono::time_point<std::chrono::steady_clock> end = begin;
    
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
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

        //Apenas faz a auto escala quando encontra o tabuleiro charuco pela primeira vez
        if(scale[0] == 0.0f){
            scale = cd.getAutoScaleVector(ourModel.getSize(), image_read, autoScaleAxis);
            step = cd.getCharucoBoardObjSize().width * 0.1;
            model = glm::scale(glm::mat4(1.0f), scale);
        }
       
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", viewMatrix);
        ourShader.setMat4("model", model);
        ourShader.setMat4("scale_matrix", scale_matrix);
        glm::vec3 camera_position = -glm::transpose(glm::mat3(viewMatrix))*glm::vec3(viewMatrix[3]);
        ourShader.setVec3("camera_position", camera_position);
        ourShader.setVec3("lightPos", lightPos[0], lightPos[1], lightPos[2]);
        ourModel.Draw(ourShader);

        glDisable(GL_DEPTH_TEST);
 
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, image_read.cols, image_read.rows, GL_BGR, GL_UNSIGNED_BYTE, image_write.data);

        flip(image_write, image_write, 0);
        
        //Applying barrel distortion        
        applyBarrelDistortion(image_write, pontos_dist);

        image_write.copyTo(output(Rect(0, 0, c.getHalfScreenWidth(), c.getHalfScreenHeight())));
	    image_write.copyTo(output(Rect(c.getHalfScreenWidth(), 0, c.getHalfScreenWidth(), c.getHalfScreenHeight())));    
	    
	    //Draw avg and "momentaneous" (last 5) framerate
	    if(debugMode){
	        frames++;	   
            end = chrono::steady_clock::now();  

            if(chrono::duration_cast<std::chrono::milliseconds>(end-before).count() > 1000){
                before = end;
                fps = frames;
                frames = 0;
            }
            
            sprintf (fps_text, "fps: %i", fps);
            putText(output, fps_text, Point(30,30), 
                FONT_HERSHEY_COMPLEX_SMALL, 0.5, Scalar(0,250,0), 1);
        }	    
	        
        c.writeImage(output);

        imshow("saida", output);
        char key = waitKey(1);
        if(key == 27)
            glfwSetWindowShouldClose(window, true);
        else
            processInputCV(key, step, model);
        

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

Mat cvMat2TexInput(Mat &img){
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

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    return window;
}

void processArgs(int argc, char** argv){
    int n_obg = 4;
    char obg[][10] = {"-pi\0", "-cc\0", "-iphost\0", "-m\0"};
    //Informa se é obrigatório mesmo com o modo debug ativado
    bool mode_d[] = {false, true, false, true};

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-d") == 0){
            cout << "Debug mode enabled" << endl;
            debugMode = true;
        }  
        else if(strcmp(argv[i], "-pi")==0 && argc > i+1){
            pi_credentials = argv[i+1];
            memset(obg[0], '\0', 10);
            i++;
        }
        else if(strcmp(argv[i], "-cc")==0 && argc > i+1){
            pathToCameraParameters = string(argv[i+1]);
            memset(obg[1], '\0', 10);
            i++;
        }
        else if(strcmp(argv[i], "-a")==0 && argc > i+1){
            autoScaleAxis = argv[i+1][0];
            i++;
        }   
        else if(strcmp(argv[i], "-c")==0 && argc > i+1){
            cameraIndex = argv[i+1][0]-'0';
            i++;
        }   
        else if(strcmp(argv[i], "-iphost")==0 && argc > i+1){
            iphost = string(argv[i+1]);
            memset(obg[2], '\0', 10);
            i++;
        }   
        else if(strcmp(argv[i], "-m")==0 && argc > i+1){
            modelPath = string(argv[i+1]);
            memset(obg[3], '\0', 10);
            i++;
        }
        else{
            cout << "Command " << argv[i] << "invalid" << endl;
            cout << "Minimal usage: ./oficial.out -d -c <path to camera parameters> -m <path to model>" << endl
                 << "Options:" << endl 
                 << "-a : [Optional] Informs the autoscale axis. Case not informed, autoscale is going to be disabled" << endl 
                 << "-d : [Optional] Debug mode" << endl
                 << "-c : [Optional] Informs index of local camera (-d) or pi camera (-pi). Default: 0" << endl 
                 << "-m : [Required] Model Path" << endl
                 << "-cc : [Required] Informs the path of camera calibration file" << endl
                 << "-pi: [Required if -d not passed] Informs user@ip of raspberry" << endl
                 << "-iphost: [Required if -d not passed] Informs ip of computer host" << endl;
            exit(-1);
        }
    }

    for(int i = 0; i < n_obg; i++){
        if(strcmp(obg[i], "\0") && !(!mode_d[i] && debugMode)){
            cout << "Parameter " << obg[i] << " is required" << endl;
            exit(1);
        }
    }
}

Mat initializeBarrelDistortionParameters(int width, int height, Mat cameraMatrix, Mat distCoefficients){
    assert(distCoefficients.cols == 4 && distCoefficients.rows == 1);

    Mat pontos(height, width, CV_32FC2), pontos_dist = Mat(height, width, CV_32FC2);
    Matx33d camMat = cameraMatrix; 
    Matx33d camMat_inv = camMat.inv(); 

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

glm::mat4 getProjetionMatrix(Mat cameraMatrix, int screenWidth, int screenHeight, double near, double far){
    return glm::mat4(
        2.0*cameraMatrix.at<double>(0, 0)/screenWidth, 0, 0, 0,
        0, 2.0*cameraMatrix.at<double>(1, 1)/screenHeight, 0, 0,
        -(1.0 - 2.0*cameraMatrix.at<double>(0, 2)/screenWidth), -(-1.0 + (2.0*cameraMatrix.at<double>(1, 2))/screenHeight), (-near-far)/(near-far), 1,  
        0, 0, 2*near*far/(near-far), 0
    );
}

void processInputCV(char key, float& step, glm::mat4& model){
    int sentido = 1, angle = 0;
    switch(key){
        case 'w':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, step, 0.0f));
            break;
        case 's':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -step, 0.0f));
            break;
        case 'd':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(step, 0.0f, 0.0f));
            break;
        case 'a':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(-step, 0.0f, 0.0f));
            break;
        case 'q':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, step));
            break;
        case 'e':
            model = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -step));
            break;
        case 'Z':
            scale_matrix = glm::scale(scale_matrix, glm::vec3(2.0f, 2.0f, 2.0f));
            break;
        case 'z':
            scale_matrix = glm::scale(scale_matrix, glm::vec3(0.5f, 0.5f, 0.5f));
            break;
        case 'i':
            sentido = -1;
        case 'I':
            angle = 90.0*sentido;                                
            model = glm::rotate(model, glm::radians((float) angle), glm::vec3(1.0, 0.0, 0.0));
            break;
        case 'j':
            sentido = -1;
        case 'J':
            angle = 90.0*sentido;                               
            model = glm::rotate(model, glm::radians((float) angle), glm::vec3(0.0, 1.0, 0.0));
            break;
        case 'k':
            sentido = -1;
        case 'K':
            angle = 90.0*sentido;                                
            model = glm::rotate(model, glm::radians((float) angle), glm::vec3(0.0, 0.0, 1.0));
            break;
    }     
}
