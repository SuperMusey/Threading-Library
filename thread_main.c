#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_COUNTER 100000000
pthread_mutex_t mutex;

pthread_t threads[5] = {0,0,0,0,0};
void *count(void *arg) {
	pthread_mutex_lock(&mutex);
	unsigned long int c = (unsigned long int)arg;
	int i;
	for (i = 0; i < c; i++) {
		if ((i % 100000) == 0) {
			printf("id: %li counted to %d of %ld and mutex held %i\n",
			       pthread_self(), i, c,mutex.__data.__lock);
		}
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}
void *count_unlock(void *arg) {
	unsigned long int c = (unsigned long int)arg;
	int i;
	for (i = 0; i < c; i++) {
		if ((i % 100000) == 0) {
			printf("id: %li counted to %d of %ld in main thread\n",
			       pthread_self(), i, c);
		}
	}
	return NULL;
}

void main(int argc, char* argv[]){
	pthread_mutex_init(&mutex,NULL);
    pthread_create(&threads[0],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
    pthread_create(&threads[1],NULL,count,(void *)(intptr_t)(2*MAX_COUNTER));
    count_unlock((void *)(intptr_t)((4) * MAX_COUNTER));
    return;
}