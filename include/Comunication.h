#ifndef COMUNICATION_CPP
#define COMUNICATION_CPP


#include <thread>
#include "./concurrentqueue.h"
#include <cstring>
#include <ctime>

//Socket
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class Comunication{
private:    
    const unsigned int ar_18_9 = 2000;
    const unsigned int ar_4_3 = 1333;
    const unsigned int ar_16_9 = 1777;    
    const unsigned int aspectRatio_heights[4] = {720, 720, 768, 480};
    const unsigned int aspectRatio_widths[4] = {1440, 1280, 1024, 640};

    bool debugMode;
    bool readDevice;
    int camera_width;
    int camera_height;
    int screen_width;
    int screen_height;
    std::string ip;
    int port;

    moodycamel::ConcurrentQueue<cv::Mat> output_queue;
    moodycamel::ConcurrentQueue<cv::Mat> input_queue;
    cv::VideoWriter out;
    cv::VideoCapture cap;    
    cv::Mat actual;
    cv::Mat outFrame;
    std::thread output_thread;    
    std::thread input_thread;   

    void* readFromDevice(){
        cv::Mat image;
        while(readDevice){
            cap >> image;
            input_queue.enqueue(image);
        }
        return 0;
    }

    void* writeToDevice(){
        cv::Mat image;
        while(readDevice){
            if(output_queue.try_dequeue(image)){
                out << image;
            }
        }
        return 0;
    }

    void* writeToDevice2(){
        int milisec = 50; // length of time to sleep, in miliseconds
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = milisec * 1000000L;

        while(readDevice){
            nanosleep(&req, (struct timespec *)NULL);
            while(this->outFrame.empty());
            out << this->outFrame;
            this->outFrame.release();            
        }

        return 0;
    }
    
    void setScreenSize(float ar){
        if(ar_18_9 == ar)
        {
            screen_width = aspectRatio_widths[0];
            screen_height = aspectRatio_heights[0];            
        }
        else if(ar_16_9 == ar)
        {
            screen_width = aspectRatio_widths[1];
            screen_height = aspectRatio_heights[1];  
        }
        else if(ar_4_3 == ar)
        {
            screen_width = aspectRatio_widths[2];
            screen_height = aspectRatio_heights[2];  
        }
        else
        {
            std::cout << "Device aspect ratio not found" << std::endl;
            screen_width = aspectRatio_widths[3];
            screen_height = aspectRatio_heights[3];  
        }

        std::cout << "width: " << screen_width << std::endl;
        std::cout << "height: " << screen_height << std::endl;
    }

    void getDeviceAspectRatio(/*std::string ip,*/ unsigned short port){        
        //Variaveis relacionadas ao socket para obter tamanho tela smartphone
        int server_sockfd, client_sockfd, opt = 1, bind_result, screen_width, screen_height;
        size_t server_len, client_len;
        struct sockaddr_in server_address;
        struct sockaddr_in client_address;
        
        //Cria um novo socket
        server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_sockfd < 0)
        {
            std::cout << "socket nao abriu" << std::endl;
            //return -1;
        }
        
        //Define que o programa pode reutilizar socket, não precisando esperar
        //tempo mínimo para reutilizá-lo
        setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
        server_address.sin_port = htons(port);        
        server_len = sizeof(server_address);
        
        bind_result = bind(server_sockfd, (struct sockaddr *) &server_address, server_len);
        if(bind_result < 0)
        {
            std::cout << "erro bind" << std::endl;
            //return -1;
        }
        
        std::cout << "Esperando conexao" << std::endl;
        listen(server_sockfd, 1);
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, (socklen_t*) &client_len);    
        read(client_sockfd, &screen_width, sizeof(int));   
        read(client_sockfd, &screen_height, sizeof(int));
        
        this->screen_width = (int) ntohl(screen_width) ;
        this->screen_height = (int) ntohl(screen_height) ;
        this->ip = std::string(inet_ntoa(client_address.sin_addr));
        std::cout << this->ip + " conectado" << std::endl;
        close(client_sockfd);    
        close(server_sockfd);
        
        int ar = screen_width*1000.0/screen_height;
        setScreenSize(0); 
    }

public:
    Comunication(unsigned short port, bool debugMode = false,
                 int width = 640, int height = 480){

        this->actual = cv::Mat(cv::Size(1280, 720), CV_8UC3, cv::Scalar(0, 0, 0));
        this->readDevice = true;

        this->debugMode = debugMode;
        this->port = port;

        if(!debugMode){
            assert(port > 0);
            getDeviceAspectRatio(port);
        }
        else{
            this->screen_width = width;
            this->screen_height = height;
        }
    }

    ~Comunication(){
        readDevice = false;
        output_thread.join();

        if(!this>debugMode)
            input_thread.join();
    }

    void openComunicators(std::string in_pipe, std::string out_pipe){
        if(!in_pipe.empty())
            cap.open(std::string(in_pipe), cv::CAP_GSTREAMER);
        else
            cap.open(0);

        if(!cap.isOpened()){
            std::cout << "Video capture not opened" << std::endl;
            throw 20;
        }

        cv::Mat img;
        cap >> img;
        this->camera_height = img.rows;
        this->camera_width = img.cols;

        if(!this->debugMode){
            out_pipe = out_pipe.replace(out_pipe.find("{width}"), 7, std::to_string(this->screen_width));
            out_pipe = out_pipe.replace(out_pipe.find("{height}"), 8, std::to_string(this->screen_height));
            out_pipe = out_pipe.replace(out_pipe.find("{ip}"), 4, this->ip);

            size_t fim = out_pipe.find('}');
            size_t inicio = out_pipe.find('{');
            size_t length = std::string::npos;

            if(fim != std::string::npos && inicio != std::string::npos){
                length=fim-inicio+1;
            }
            else if(inicio == std::string::npos && fim == std::string::npos){
                inicio = 0;
                length = 0;
            }
            inicio = (inicio == std::string::npos) ? 0 : inicio;

            std::string error = out_pipe.substr(inicio, length);
            if(!error.empty()){
                std::cout << "Cannot map " << error << std::endl;
                throw -1;
            }

            std:: cout << "outpipe: " << out_pipe << std::endl;
            out.open(std::string(out_pipe), cv::CAP_GSTREAMER, 20, cv::Size(this->camera_width, this->camera_height));
            if(!out.isOpened()){
                std::cout << "Video writer not opened" << std::endl;
            }
        } 
    }    
    
    void startComunication(){
        this->readDevice = true;
        this->input_thread = std::thread(&Comunication::readFromDevice, this);

        if(!this->debugMode)
            this->output_thread = std::thread(&Comunication::writeToDevice2, this);
    }

    int getWidth(){
        return camera_width;
    }

    int getHeight(){
        return camera_height;
    }

    cv::Mat readImage(){
        cv::Mat newActual;
        if(input_queue.try_dequeue(newActual))
            this->actual = newActual.clone();

        return this->actual;
    }

    void writeImage(cv::Mat image){
        //if(!this->debugMode)
        //    output_queue.enqueue(image);        
        if(this->outFrame.empty()){
            this->outFrame = image.clone();
            //cv::imshow("debug1", this->outFrame);
            //if(waitKey(10)==27) throw 20;
        }
        
    }
};


#endif /* !FILE_FOO_SEEN */
