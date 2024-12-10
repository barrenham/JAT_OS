#include "syscall.h"
#include "stdio.h"

int main(){
    printf("I'm the first visitor!\n");
    for(int i=0;i<3;i++){
        int32_t pid=fork();
        if(pid==0){
            printf("I'm father!\n");
            while(1);
            process_exit();
        }
    }
    while(1);
    process_exit();
    return 0;
}