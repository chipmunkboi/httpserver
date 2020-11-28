#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//global
pthread_mutex_t lock;

void* trythis(void* arg){
    pthread_mutex_lock(&lock); 
    int *counter = (int*)arg;      

    // unsigned long i = 0;
    *counter += 1;                  
    int my_counter = *counter;      
    pthread_mutex_unlock(&lock); 
    printf("\nJob %d has started\n", my_counter);

    sleep(3); //do stuff

    printf("\nJob %d has finished\n", my_counter);
    
    return NULL;
}

int main(void){
    pthread_t tid[2];
    int counter = 0;
    int i = 0;
    int error;

    //init mutex
    if(pthread_mutex_init(&lock, NULL) != 0 ){
        printf("\nmutex init has failed\n");
        return 1;
    }

    //create 2 threads
    while (i < 2){
        error = pthread_create(&(tid[i]), NULL, &trythis, &counter);
        if(error != 0){
            printf("\nThread can't be created: [%s]", strerror(error));
        }

        i++;
    }
    pthread_cond_wait(&tid[0],&lock);
    while(1){

    }

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    return 0;
}