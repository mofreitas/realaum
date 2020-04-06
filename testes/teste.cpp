#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

void resize(GLFWwindow* janela, int width, int height){
    glViewport(0, 0, width, height);
}

void lidaTeclado(GLFWwindow* janela){
    if(glfwGetKey(janela, GLFW_KEY_ESCAPE)==GLFW_PRESS){
        glfwSetWindowShouldClose(janela, true);
    }
}

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* janela = glfwCreateWindow(640, 480, "olá opengl", NULL, NULL);
    if(janela==NULL){
        cout << "Erro ao criar janela" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(janela);

    //Inicia o GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        cout << "Não inicializou o glad" << endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(janela, resize);
    glClearColor(0.2, 0.3, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    while(!glfwWindowShouldClose(janela)){
        lidaTeclado(janela);
        glfwSwapBuffers(janela);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
