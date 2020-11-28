#include <stdio.h> 
// #include <iostream>
#include <stdlib.h> 
#include <unistd.h> //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 
#include <queue>
using namespace std;
queue <int> commQ;

struct synchronization{
    // Declaration of thread condition variable 
    int newReq = 0; 
    // declaring mutex 
    int queueLock = 1; 
};

// A normal C function that is executed as a thread 
// when its name is specified in pthread_create() 
void *myThreadFun(void *vargp) 
{ 
	sleep(2); 
    int print = commQ.front();
    commQ.pop();
	printf("%d\n", print); 
	return NULL; 
} 

int main() 
{ 
    commQ.push(5);
    commQ.push(4);
    commQ.push(53);
    commQ.push(2);
    commQ.push(1);
    struct synchronization sink;
	pthread_t thread_id; 
    pthread_t thread2;
	printf("Before Thread\n"); 
	pthread_create(&thread_id, NULL, myThreadFun, (void*)(&sink));
    pthread_create(&thread2, NULL, myThreadFun, (void*)(&sink));
    pthread_join(thread_id, NULL); 
    pthread_join(thread2, NULL);
	printf("After 2 Thread\n"); 
    commQ.pop();
	exit(0); 
}
