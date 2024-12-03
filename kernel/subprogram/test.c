#include "syscall.h"
#include "stdio.h"

int main(){
    printf("I'm the first visitor!\n",NULL);
    process_exit();
    return 0;
}