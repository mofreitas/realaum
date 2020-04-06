#include <iostream>
#include <pthread.h>
#include <concurrentqueue.h>
#include <cstdlib>

using namespace std;

bool finish = false;
unsigned int size_thread = 0;
moodycamel::ConcurrentQueue<int> queue; 

void* thread_function(void* a){
    int* number = (int*)malloc(sizeof(int));
    for(int i =0 ;i < 100; i++){
        *number = i*100;
        queue.enqueue(*number);
        size_thread++;
    }
    finish = true;
}

int main(){  

    pthread_t produtor; 

    pthread_create(&produtor, NULL, thread_function, (void*)0);

    int saida;
    unsigned int size_main = 0;
    while(!(finish && size_thread == size_main)){
        if(queue.try_dequeue(saida)){
            cout << saida << endl;
            size_main++;      
        }          
        else 
            cout << "FAILED" << endl;
        
    }



    return 0;
}