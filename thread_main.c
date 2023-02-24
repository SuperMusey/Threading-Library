#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

pthread_t threads[2] = {0,0};

void *count(void *id){
    printf("In thread %li\n",(unsigned long int)id);
}

void main(int argc, char* argv[]){
    pthread_create(&threads[0],NULL,count,(void *)(intptr_t)1);
    return;
}