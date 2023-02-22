#include "ec440threads.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define TEST_STACK 64
#define SIZE_OF_ADDRESS 48 //in bits

/* stack growth and function storage test */

void stk_fnc(void){
    printf("stk_func called\n");
    return;
}

void main(void){
    void* stack = malloc(TEST_STACK);
    void* stackPointer;
    stackPointer = TEST_STACK+stack-8;
    printf("Address of stack function:%p\n",stk_fnc);
    printf("Address of stack:%p\n",stack);
    printf("Address of stackPointer:%p\n",stackPointer);
    *(unsigned long int *)stackPointer = (unsigned long int)&stk_fnc;
    //void (*f)(void);
    //f = *(unsigned long int *)stackPointer;
    //f();
    return;
}