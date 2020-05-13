#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
//include <opencv2/videoio.hpp>
//#include <opencv2/imgproc.hpp>


using namespace cv;

const char* shader = 
"#version 330 core\n"
"layout (location = 0) in vec3 vec;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"void main(){\n"
"   gl_Position = vec4(vec.x, vec.y, vec.z, 1.0);\n"
"   TexCoord = aTexCoord;\n"
"}\0";

const char* fragShader = 
"#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D aTexture;\n"
"void main(){\n"
"    FragColor = texture(aTexture, TexCoord);\n"
"}\0";

const char* shader2 = 
"#version 330 core\n"
"layout (location = 0) in vec3 vec;\n"
"uniform mat4 proj;\n"
"uniform mat4 view;\n"
"void main(){\n"
"    gl_Position = proj * view * vec4(vec.x, vec.y, vec.z, 1.0);\n"
"}\0";

const char* fragShader2 =
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main(){\n"
"    FragColor = vec4(1.0f, 1.0f, 0.2f, 1.0f);\n"
"}\0";


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned char* cvMat2TexInput(Mat& img);


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for Mac OSX
#endif

    VideoCapture cap;
    cap.open(0);
    if (!cap.isOpened())
    {
        std::cout << "Camera not opened!" << std::endl;
        return -1;
    }
    //cap.set(cv::CAP_PROP_FRAME_WIDTH,640);
    //cap.set(cv::CAP_PROP_FRAME_HEIGHT,480);

    Mat frame;
    cap >> frame;
    int width = frame.cols;
    int height = frame.rows;
    unsigned char* image = cvMat2TexInput(frame);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(width, height, "frame", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Shader ourShader("shader.vert", "shader.frag");
    float vertices[] = {
        //     Position       TexCoord
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, // top left
        1.0f,   1.0f, 0.0f, 1.0f, 1.0f, // top right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // below left
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f  // below right 
    };
    // Set up index
    unsigned int indices[] = {
        0, 1, 2,
        1, 2, 3
    };

    float vertices2[]={-0.5, -0.5, 1.0,
                     0.5, -0.5, 1.0,
                     0, 0.5, 1.0};

    //Define Shaders
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &shader, NULL);
    glCompileShader(vertexShader);  

    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);  
    if(!success){
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShader, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);  
    if(!success){
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);


    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);  

    glGenBuffers(1, &VBO);  
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    unsigned int aTexture;
    glGenTextures(1, &aTexture);
    glBindTexture(GL_TEXTURE_2D, aTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    //--------------------------------------------------------------------------------------------

    //Define Shaders
    unsigned int vertexShader2;
    vertexShader2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader2, 1, &shader2, NULL);
    glCompileShader(vertexShader2);  

    glGetShaderiv(vertexShader2, GL_COMPILE_STATUS, &success);  
    if(!success){
        glGetShaderInfoLog(vertexShader2, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader2;
    fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader2, 1, &fragShader2, NULL);
    glCompileShader(fragmentShader2);

    glGetShaderiv(fragmentShader2, GL_COMPILE_STATUS, &success);  
    if(!success){
        glGetShaderInfoLog(fragmentShader2, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram2;
    shaderProgram2 = glCreateProgram();
    glAttachShader(shaderProgram2, vertexShader2);
    glAttachShader(shaderProgram2, fragmentShader2);
    glLinkProgram(shaderProgram2);

    unsigned int VAO2, VBO2;
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);  

    glGenBuffers(1, &VBO2);  
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), &vertices2, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);

    glm::mat4 view;
    view = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), 
  		   glm::vec3(0.0f, 0.0f, 0.0f), 
  		   glm::vec3(0.0f, 1.0f, 0.0f));

     

    int projLocation = glGetUniformLocation(shaderProgram2, "proj");
    int viewLocation = glGetUniformLocation(shaderProgram2, "view");

    glEnable(GL_DEPTH_TEST);        

    while(!glfwWindowShouldClose(window))
    {
        cap >> frame;
        image = cvMat2TexInput(frame);
        if(image)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        else
        {
            std::cout << "Failed to load texture." << std::endl;
        }

        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        // Draw Rectangle
        //ourShader.use();
        glBindTexture(GL_TEXTURE_2D, aTexture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glEnable(GL_DEPTH_TEST); 

        glUseProgram(shaderProgram2);

        glUniformMatrix4fv(projLocation, 1, 0, glm::value_ptr(proj));
        glUniformMatrix4fv(viewLocation, 1, 0, glm::value_ptr(view));

        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

unsigned char* cvMat2TexInput(Mat& img)
{
    cvtColor(img, img, cv::COLOR_BGR2RGB);
    flip(img, img, -1);
    return img.data;
}
