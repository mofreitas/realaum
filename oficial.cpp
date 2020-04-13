#include <Comunication.h>
#include <charuco/CharucoDetector.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"

using namespace cv;

GLFWwindow* initializeProgram(GLuint width, GLuint height);
void processArgs(int argv, char** argc);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned char *cvMat2TexInput(Mat &img); 

// settings
bool debugMode = false;

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

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0,1.0, 0.0));
float lastX = 0;
float lastY = 0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main(int argc, char** argv)
{    
    processArgs(argc, argv);

    Comunication c(40001, debugMode);
    c.openComunicators(
        "v4l2src device=/dev/video0 ! video/x-raw, width=640 ! videorate max-rate=20 ! videoconvert ! queue ! appsink",
        /*"udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96, a-framerate=20 ! rtpjitterbuffer drop-on-latency=true ! rtph264depay ! queue ! decodebin ! videoconvert ! queue ! appsink*/
        "appsrc is-live=true ! queue ! videoscale ! video/x-raw, width={width}, height={height} ! videoconvert ! video/x-raw, format=I420 ! queue ! vaapih264enc quality-level=7 keyframe-period=20 ! video/x-h264, stream-format=avc ! queue ! rtph264pay config-interval=1 ! udpsink host={ip} port=5000");

    CharucoDetector cd("./cameraParameters.txt");

    GLFWwindow* window = initializeProgram((GLuint)c.getWidth(), (GLuint)c.getHeight());
    unsigned char *image;    

    Shader background_shader("./background_shader.vs", "./background_shader.fs");

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

    // configure global opengl state
    // -----------------------------
    //glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    // load models
    // -----------
    Model ourModel("./resources/objects/table2/Table.obj");

    c.startComunication();
    Mat img(c.getHeight(), c.getWidth(), CV_8UC3), frame;
    glm::mat4 eye(1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        background_shader.use();
        glActiveTexture(GL_TEXTURE0);
        background_shader.setInt("aTexture", 0);
        glBindTexture(GL_TEXTURE_2D, aTexture);

        frame = c.readImage();        
        glm::mat4 viewMatrix = cd.get_charuco_extrinsics(frame);
        image = cvMat2TexInput(frame);         

        if (image)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, c.getWidth(), c.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        else
        {
            std::cout << "Failed to load texture." << std::endl;
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //RESETA BIND
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)c.getWidth() / (float)c.getHeight(), 10.0f, 1000.0f);
        glm::mat4 model = glm::scale(eye, glm::vec3(0.01f, 0.01f, 0.01f)); 
       
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", viewMatrix);
        ourShader.setMat4("model", model);
        ourShader.setVec3("viewPos", glm::vec3(viewMatrix[0][3], viewMatrix[1][3], viewMatrix[2][3]));
        ourModel.Draw(ourShader);

        glDisable(GL_DEPTH_TEST);

        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
        c.writeImage(img);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
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

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif   

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(width, height, "Realaum", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    return window;
}

void processArgs(int argc, char** argv){
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-d") == 0){
            cout << "Modo debug ativo" << endl;
            debugMode = true;
        }  
        else{
            cout << "Comando " << argv[i] << "inválido" << endl;
            exit(-1);
        }
    }
}

