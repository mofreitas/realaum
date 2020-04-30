#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>

using namespace std;

void runScript(std::string command, bool wait_finish = true)
{
    pid_t pid = fork();
    if (pid == 0){  
        std::cout << "Running: " << std::endl
                  << command << std::endl;  
          
        execl("/bin/bash", "bash", "-c", command.c_str(), (char *)0);
        perror("Error: Cannot execute command");
    }
    else if (pid > 0){
        if (wait_finish){
            waitpid(pid, NULL, 0);
        }
    }
    else{
        std::cout << "Error: Cannot fork proccess to run command below" << std::endl
                  << command << std::endl;
        throw - 1;
    }
}

int main(){

    std::string comando1 = "./waitOutput.sh ./output_gst.txt \"REPRODUZINDO|PLAYING\"";
    std::string comando2 = "ssh pi@192.168.0.21 -t 'gst-launch-1.0 -vvv videotestsrc ! videoconvert ! video/x-raw ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! queue ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=192.168.0.16 2>&1' > ./output_gst.txt";
    
    runScript(comando2, false);
    runScript(comando1, false);
    std::cout << "sleeee" << std::endl;
    sleep(20);
    kill(0, SIGHUP);
}