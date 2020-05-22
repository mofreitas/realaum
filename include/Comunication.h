#ifndef COMUNICATION_CPP
#define COMUNICATION_CPP

#include <thread>
#include "./concurrentqueue.h"
#include <cstring>
#include <ctime>
#include <mutex>
#include <regex>
#include <cassert>

//Fork
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

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
    struct Resolution{
        int width;
        int height;

        Resolution(){}

        Resolution(int w, int h){
            width = w;
            height = h;
        }
    };

    const unsigned int ar_19_9 = 2111;  
    const unsigned int ar_18_9 = 2000;
    const unsigned int ar_4_3 = 1333;
    const unsigned int ar_16_9 = 1777;    
    const unsigned int aspectRatio_heights[4] = {360, 360, 480, 360};
    const unsigned int aspectRatio_widths[4] = {720, 640, 640, 760};

    bool debugMode;
    std::atomic<bool> readDevice;
    Resolution camera;
    Resolution frame;
    Resolution screen;
    int framerate;
    std::string device_ip;
    std::string host_ip;
    std::mutex output_lock;
    int port;

    moodycamel::ConcurrentQueue<cv::Mat> output_queue;
    moodycamel::ConcurrentQueue<cv::Mat> input_queue;
    cv::VideoWriter out;
    cv::VideoCapture cap;    
    cv::Mat actual;
    cv::Mat outFrame;
    std::thread output_thread;    
    std::thread input_thread;  
    std::thread thread_leitura;
    std::mutex mutex;

    void* readFromDevice(){
        //camera size > frame size <= screen size
        int half_screen_width = this->screen.width/2;
        int half_screen_height = this->screen.height;
        int border_h_width = (this->camera.width - this->frame.width)/2;
        int border_h_height = (this->camera.height - this->frame.height)/2;
        cv::Mat image, output(half_screen_height, half_screen_width, CV_8UC3, cv::Scalar(0, 0, 0));

        //Get central section of output
        cv::Rect src_rect(border_h_width, border_h_height, this->frame.width, this->frame.height);
        cv::Rect dst_rect((half_screen_width - this->frame.width)/2, (half_screen_height - this->frame.height)/2, this->frame.width, this->frame.height);

        while(readDevice.load()){
            cap >> image;
            image(src_rect).copyTo(output(dst_rect));
            input_queue.enqueue(output);
        }
        return 0;
    }

    void* writeToDevice(){
        cv::Mat image;
        while(readDevice.load()){
            if(output_queue.try_dequeue(image)){
                out << image;
            }
        }
        return 0;
    }

    void* writeToDevice2(){
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 1e9/this->framerate;
        std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

        while(readDevice){
            nanosleep(&req, (struct timespec *)NULL);
            std::lock_guard<std::mutex> lock(mutex);
                out << this->outFrame;
                //this->outFrame.release();    
        }

        return 0;
    }
    
    /** Mapeia o aspect ratio enviado em aspect ratios predeterminados
     * 
     * @param ar Aspect ratio que deseja mapear 
     */
    void setScreenSize(float ar){
        if(ar_18_9 == ar)
        {
            screen.width = aspectRatio_widths[0];
            screen.height = aspectRatio_heights[0];            
        }
        else if(ar_16_9 == ar)
        {
            screen.width = aspectRatio_widths[1];
            screen.height = aspectRatio_heights[1];  
        }
        else if(ar_4_3 == ar)
        {
            screen.width = aspectRatio_widths[2];
            screen.height = aspectRatio_heights[2];  
        }
        else if(ar_19_9 == ar)
        {
            screen.width = aspectRatio_widths[3];
            screen.height = aspectRatio_heights[3];  
        }
        else
        {
            std::cout << "Device aspect ratio not found" << std::endl;
            screen.width = aspectRatio_widths[2];
            screen.height = aspectRatio_heights[2];  
        }

        std::cout << "Output width x height: " << screen.width 
                  << "x" << screen.height << std::endl;
    }

    /**
     * Obtem aspect ratio do dispositivo, dado que envie a largura e altura da tela.
     * 
     * @param port Porta em que o programa estará esperando conexão do dispositivo móvel
     */
    void getDeviceAspectRatio(unsigned short port){        
        //Variaveis relacionadas ao socket para obter tamanho tela smartphone
        int server_sockfd, client_sockfd, opt = 1, bind_result, screen_width, screen_height;
        size_t server_len, client_len;
        struct sockaddr_in server_address;
        struct sockaddr_in client_address;
        
        //Cria um novo socket
        server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_sockfd < 0)
        {
            std::cout << "socket not open" << std::endl;
            throw -1;
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
            std::cout << "Error binding port to address" << std::endl;
            throw -1;
        }

        std::cout << "Waiting connection on port " << this->port << std::endl;
        listen(server_sockfd, 1);
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, (socklen_t*) &client_len);    
        read(client_sockfd, &screen_width, sizeof(int));   
        read(client_sockfd, &screen_height, sizeof(int));
        
        this->device_ip = std::string(inet_ntoa(client_address.sin_addr));
        std::cout << this->device_ip + " connected" << std::endl;
        close(client_sockfd);    
        close(server_sockfd);
        
        int ar = screen_width*1000.0/screen_height;
        setScreenSize(ar); 
    }

    /**
     * Função que substitui os parâmetros em uma string por seus valores, fazendo ainda a verificação (se há chaves faltantes) da string de entrada
     * 
     * @param parameters Lista de palavras chaves entre chaves que a string *pode* ter
     * @param values Lista dos respectivos valores que cada parâmetro deve assumir
     * @param string String que originalmente possui os parâmetros e, no final, será a string com os parâmetros substituídos pelos seus respectivos valores
     */
    void insertParameters(std::vector<std::string> parameters, std::vector<std::string> values, std::string& string_){
        assert(parameters.size() == values.size());
      
        for(int i = 0; i < parameters.size(); i++){
            size_t found = string_.find(parameters.at(i));
            if(found != std::string::npos)
                string_.replace(found, parameters.at(i).size(), values.at(i));
        }

        size_t fim = string_.find('}');
        size_t inicio = string_.find('{');
        size_t length = std::string::npos;

        if(fim != std::string::npos && inicio != std::string::npos){
            length=fim-inicio+1;
        }
        else if(inicio == std::string::npos && fim == std::string::npos){
            inicio = 0;
            length = 0;
        }
        inicio = (inicio == std::string::npos) ? 0 : inicio;

        std::string error = string_.substr(inicio, length);
        if(!error.empty()){
            std::cout << "Cannot map " << error << std::endl;
            throw -1;
        }
    }

    /**
     * Executa comando no shell. 
     * 
     * @param command Comando a ser executado no shell
     * @param timeout Tempo de espera para comando ser concluído antes de cancelá-lo. Se timeout < 0, espera indefinidamente, se timeout == 0, 
     * script não bloqueia a execução do programa, se timeout > 0, espera timeout segundos para concluir.
     */
    void runScript(std::string command, int timeout = 0){
        pid_t pid = fork();

        //Se processo filho
        if (pid == 0){  
            std::cout << "Running: " << command << std::endl;     
            execl("/bin/bash", "bash", "-c", command.c_str(), (char *)0);
            //perror("Error: Cannot execute command");
            std::cout << "Error: Cannot execute command" << std::endl;
        }
        //Se processo pai
        else if (pid > 0){
            if (timeout < 0){
                waitpid(pid, NULL, 0);
            }
            else if(timeout > 0){
                sleep(timeout);
                if(waitpid(pid, NULL, WNOHANG) == 0){
                    std::cout << "Timeout Error: Command no finished in " << timeout << " seconds: " << command << std::endl;
                    throw -1;
                }
            }
        }
        else{
            std::cout << "Error: Cannot fork proccess to run command: "
                      << command << std::endl;
            throw -1;
        }
    }
    
public:
    Comunication(std::string iphost, unsigned short port, bool debugMode = false,
                 int cameraWidth = 640, int cameraHeight = 480){

        this->readDevice = true;

        this->debugMode = debugMode;
        this->port = port;
        std::regex valid_ip("\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");

        if(!debugMode){
            assert(std::regex_match (iphost, valid_ip));
            this->host_ip = iphost;
            assert(port > 0);
            getDeviceAspectRatio(port);
        }
        else{
            this->screen.width = cameraWidth;
            this->screen.height = cameraHeight;
        }

        this->actual = cv::Mat(cv::Size(screen.width/2, screen.height), CV_8UC3, cv::Scalar(0, 0, 0));
        this->outFrame = cv::Mat(cv::Size(screen.width, screen.height), CV_8UC3, cv::Scalar(0, 0, 0));

        this->camera.width = cameraWidth;
        this->camera.height = cameraHeight;

        this->frame.width = cameraWidth <= this->screen.width/2 ? cameraWidth : this->screen.width/2;
        this->frame.height = cameraHeight <= this->screen.height ? cameraHeight : this->screen.height;
    }

    ~Comunication(){
        readDevice = false;
        kill(0, SIGHUP);

        input_thread.join();
        if(!this>debugMode)
            output_thread.join();
    }
    
    
    /**
     * Abre fluxos de entrada e saída de dados do programa
     * 
     * @param outpipe Define o pipe **GSTREAMER** que indica
     * @param pi_ssh_login Informa o login e ip da raspberry seguindo o padrão de acesso pelo terminal: login\@ip
     * @param in_pipe Informa o pipe **GSTREAMER** de entrada do programa. Por padrão, ele obtem da câmera /dev/video0 conectada ao PC.
     * @param framerate Informa o framerate de funcionamento do programa. Ele deve ser igual em todos os pipes
     */
    void openComunicators(std::string out_pipe, std::string pi_ssh_login = std::string(), std::string in_pipe = std::string(), int framerate = 20){
        this->framerate = framerate;

        if(!this->debugMode && !pi_ssh_login.empty()){
            std::string device_pipe = "ssh {pi_ssh_login} -t 'gst-launch-1.0 -vvv v4l2src device=/dev/video0 ! videoconvert ! video/x-raw, width={width}, height={height} ! videorate ! video/x-raw, framerate={framerate}/1 ! queue ! omxh264enc ! queue ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host={host}' > ./output_gst.txt";
            insertParameters({"{framerate}", "{ip}", "{pi_ssh_login}", "{width}", "{height}", "{host}"}, 
                            {std::to_string(framerate), this->device_ip, pi_ssh_login, std::to_string(camera.width), std::to_string(camera.height), this->host_ip},
                            device_pipe);

            runScript(device_pipe);

            runScript("./waitOutput.sh ./output_gst.txt \"REPRODUZINDO|PLAYING\"", 10);
        }

        //Abrindo pipe de entrada
        if(pi_ssh_login.empty() || in_pipe.empty() || this->debugMode){
            in_pipe = "v4l2src device=/dev/video0 ! videoscale ! video/x-raw, width=640 ! videorate max-rate={framerate} ! videoconvert ! queue ! appsink";
        }

        insertParameters({"{framerate}"}, 
                         {std::to_string(framerate)},
                          in_pipe); 

        std:: cout << "in_pipe: " << in_pipe << std::endl; 
        cap.open(in_pipe, cv::CAP_GSTREAMER);
        if(!cap.isOpened()){
            std::cout << "Video capture not opened" << std::endl;
            throw -1;
        }

        std::cout << "Camera size source: " << this->camera.width << "x" << this->camera.height << std::endl;

        //Abrindo pipe de saída
        if(!this->debugMode){
            insertParameters({"{width}", "{height}", "{ip}"}, 
                             {std::to_string(this->screen.width), std::to_string(this->screen.height), this->device_ip},
                             out_pipe);           

            std:: cout << "outpipe: " << out_pipe << std::endl;
            out.open(std::string(out_pipe), cv::CAP_GSTREAMER, 20, cv::Size(this->screen.width, this->screen.height));
            if(!out.isOpened()){
                std::cout << "Video writer not opened" << std::endl;
                throw - 1;
            }
        } 
    }    
    
    /**
     * Inicia threads que fazem a comunicação com dispositivos externos.
     */
    void startComunication(){
        this->readDevice = true;
        this->input_thread = std::thread(&Comunication::readFromDevice, this);

        if(!this->debugMode)
            this->output_thread = std::thread(&Comunication::writeToDevice2, this);
    }

    int getCameraWidth(){
        return camera.width;
    }

    int getCameraHeight(){
        return camera.height;
    }

    int getFrameWidth(){
        return frame.width;
    }

    int getFrameHeight(){
        return frame.height;
    }

    int getScreenWidth(){
        return screen.width;
    }

    int getScreenHeight(){
        return screen.height;
    }

    int getHalfScreenWidth(){
        return screen.width/2;
    }

    int getHalfScreenHeight(){
        return screen.height;
    }

    cv::Mat readImage(){
        cv::Mat newActual;
        if(input_queue.try_dequeue(newActual))
            this->actual = newActual.clone();

        return this->actual;
    }

    void writeImage(cv::Mat image){
        std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
        if(lock.try_lock()){            
            this->outFrame = image.clone();
            lock.unlock();
        }        
    }
};


#endif /* !FILE_FOO_SEEN */
