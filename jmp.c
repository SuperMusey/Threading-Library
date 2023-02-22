#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

jmp_buf buf;

void jmp_func(void){
    printf("jmp_func executing......\n");
    longjmp(buf,1);
    return;
}

void main(void){
    if(setjmp(buf)==0){
        jmp_func();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
    }else{
        printf("Jumped\n");
    }
    return;
}