#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_COUNTER 100000000
pthread_mutex_t mutex;
pthread_barrier_t barrier;

int glb_inc = 0;

pthread_t threads[5] = {0,0,0,0,0};
void *count(void *arg) {
	int i;
	for (i = 0; i < 1000; i=i+50) {
			printf("id: %li counted to %d of %i and before barrier\n",
			       pthread_self(), i, 1000);
	}
	pthread_barrier_wait(&barrier);
	unsigned long int c = (unsigned long int)arg;
	int j;
	for (j = 0; j < c; j++) {
		if ((j % 100000) == 0) {
			printf("id: %li counted to %d of %ld\n",
			       pthread_self(), j, c);
		}
	}
	//pthread_mutex_unlock(&mutex);
	return NULL;
}
void *count_unlock(void *arg) {
	unsigned long int c = (unsigned long int)arg;
	int i;
	for (i = 0; i < c; i++) {
		if ((i % 100000) == 0) {
			// printf("id: %li counted to %d of %ld in main thread\n",
			//        pthread_self(), i, c);
		}
	}
	return NULL;
}

void *increment(void* arg){
	glb_inc++;
	pthread_barrier_wait(&barrier);
	printf("thread %li counted %i\n",pthread_self(),glb_inc);
	return NULL;
}

void main(int argc, char* argv[]){
	// pthread_mutex_init(&mutex,NULL);
	// pthread_barrier_init(&barrier,NULL,3);
    // pthread_create(&threads[0],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
    // pthread_create(&threads[1],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
	// pthread_create(&threads[2],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
	// pthread_create(&threads[3],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
    //count_unlock((void *)(intptr_t)((6) * MAX_COUNTER));

	pthread_barrier_init(&barrier,NULL,11);
	for(int i=0;i<10;i++){
		pthread_create(&threads[0],NULL,increment,(void*)1);
	}
	pthread_barrier_wait(&barrier);
	count_unlock((void *)(intptr_t)((6) * MAX_COUNTER));
    return;
}