#include <pthread.h>
#include <stdio.h>

#define TOTAL 100000000

int z = 0;

void *inc_x(void *x_void_ptr){
    int *x_ptr = (int*)x_void_ptr;
    int *num = (int*)num;
    while(++(*x_ptr) < TOTAL);
    printf("%d increment finished\n", z);
    z++;
    return NULL;
}

int main(){
    int x = 0, y = 0, z = 0, o = 1;
    printf("x: %d, y: %d\n", x, y);

    pthread_t inc_x_thread;
    pthread_t inc_y_thread;

    if(pthread_create(&inc_x_thread, NULL, inc_x, &x)){
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(pthread_create(&inc_y_thread, NULL, inc_x, &y)){
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    // while(++y < TOTAL);
    // printf("y increment finished \n");

    if(pthread_join(inc_x_thread, NULL)){
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    if(pthread_join(inc_y_thread, NULL)){
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    printf("x: %d, y: %d\n", x, y);

    return 0;
}