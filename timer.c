#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

void alrm_action(int signum){
    printf("Alarm rcvd\n");
    return;
}

void main(void){
    struct sigaction new;
    new.sa_flags = SA_NODEFER;
    new.sa_handler = alrm_action;
    sigaction(SIGALRM,&new,0);

    printf("Setup of alarm\n");
    ualarm(5*100*1000,5*100*1000);
    printf("Just waiting.....\n");
    while(1){}
    return;
}