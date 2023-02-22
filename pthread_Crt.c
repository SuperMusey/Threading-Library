#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

//http://www.csl.mtu.edu/cs4411.ck/common/Coroutines.pdf
//Multithread
int glob_test = 0;
void *thread_func(void *args){
    //sleep(1);
    int *tid = (int *)args; 
    int test_var = 0;
    printf("In thread %i has val %i\n",*tid,++glob_test);
    return NULL;
}

int main(){
    pthread_t thread_id;
    printf("Before thread\n");
    for(int i=0;i<3;i++){
        pthread_create(&thread_id,NULL,thread_func,(void *)&thread_id);
    }
    //pthread_join(thread_id,NULL);
    printf("After thread\n");
    pthread_exit(NULL);
    return 0;
}