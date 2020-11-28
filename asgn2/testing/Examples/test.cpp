#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct args {
    char *name = "Allen";
    int age = 20;
};

void *hello(void *input) {
    char *name = ((struct args*)input)->name;
    int age = ((struct args*)input)->age;
    printf("name: %s\n", name);
    printf("age: %d\n", age);
}

int main() {
    // struct args *Allen = (struct args *)malloc(sizeof(struct args));
    // char allen[] = "Allen";
    // Allen->name = allen;
    // Allen->age = 20;
    struct args allen;
    struct args emily;
    
    pthread_t tid, tid2;
    pthread_create(&tid, NULL, hello, (void *)&allen);
    pthread_create(&tid2, NULL, hello, (void *)&emily);
    pthread_join(tid, NULL);
    pthread_join(tid2, NULL);
    return 0;
}