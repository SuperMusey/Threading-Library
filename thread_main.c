#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_COUNTER 100000000

pthread_t threads[5] = {0,0,0,0,0};
void *count(void *arg) {
	unsigned long int c = (unsigned long int)arg;
	int i;
	for (i = 0; i < c; i++) {
		if ((i % 100000) == 0) {
			printf("id: %li counted to %d of %ld\n",
			       pthread_self(), i, c);
		}
	}
	return NULL;
}

void main(int argc, char* argv[]){
    pthread_create(&threads[0],NULL,count,(void *)(intptr_t)(1*MAX_COUNTER));
    pthread_create(&threads[1],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
    pthread_create(&threads[2],NULL,count,(void *)(intptr_t)(3*MAX_COUNTER));
    pthread_create(&threads[3],NULL,count,(void *)(intptr_t)(4*MAX_COUNTER));
    pthread_create(&threads[4],NULL,count,(void *)(intptr_t)(5*MAX_COUNTER));
    count((void *)(intptr_t)((6) * MAX_COUNTER));
    return;
}