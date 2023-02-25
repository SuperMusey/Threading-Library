#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_COUNTER 10

pthread_t threads[5] = {0,0,0,0,0};

void *count(void *id){
    int i=0;
    while(i!=MAX_COUNTER){
        printf("In thread %li with value of i:%i\n",pthread_self(),i);
        i++;
        sleep(1);
    }
}

void main(int argc, char* argv[]){
    pthread_create(&threads[0],NULL,count,(void *)(intptr_t)1);
    pthread_create(&threads[1],NULL,count,(void *)(intptr_t)2);
    pthread_create(&threads[2],NULL,count,(void *)(intptr_t)3);
    pthread_create(&threads[3],NULL,count,(void *)(intptr_t)4);
    pthread_create(&threads[4],NULL,count,(void *)(intptr_t)5);
    int i = 10;
    while(1){printf("in main\n");sleep(1);}
    // while(i!=100){
    //     i=i+10;
    //     printf("In main\n");
    //     sleep(1);
    // }
    return;
}