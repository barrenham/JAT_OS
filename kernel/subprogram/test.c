#include "syscall.h"
#include "stdio.h"

int main(){
    printf("I'm trying to get a higher privelege!\n");
    uint32_t ptr=0xe0000000;
    int fd=openFile("/hole.txt",O_RDWR);
    ptr+=0x1000;
    ptr-=16;
    uint32_t cs0=((1<<3)+(0<<2)+0);
    write(fd,&cs0,4);
    seekp(fd,0,SEEK_SET);
    read(fd,ptr,4);

    printf("%d\n",(*(uint32_t*)0xE0000000));
    closeFile(fd);
    process_exit();
    return 0;
}