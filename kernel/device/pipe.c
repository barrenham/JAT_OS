#include "../include/ioqueue.h"
#include "../include/fs.h"

extern struct file file_table[MAX_FILE_OPEN];

bool is_pipe(uint32_t local_fd){
    uint32_t global_fd=fd_local2global(local_fd);
    if(global_fd<0){
        return False;
    }
    return file_table[global_fd].fd_flag==PIPE_FLAG;
}

int32_t sys_pipe(int32_t pipefd[2]){
    int32_t global_fd=get_free_slot_in_global();
    file_table[global_fd].fd_inode=get_kernel_pages(1);
    ioqueue_init((struct ioqueue*)file_table[global_fd].fd_inode);
    if(file_table[global_fd].fd_inode==NULL){
        return -1;
    }
    file_table[global_fd].fd_flag=PIPE_FLAG;
    file_table[global_fd].fd_pos=2; //cite from book, 复用为打开次数
    pipefd[0]=pcb_fd_install(global_fd);
    pipefd[1]=pcb_fd_install(global_fd);
    return 0;
}

uint32_t pipe_read(int32_t fd,void* buf,uint32_t count){
    char* buffer=buf;
    uint32_t bytes_read=0;
    int32_t global_fd=fd_local2global(fd);

    struct ioqueue* ioq=(struct ioqueue*)file_table[global_fd].fd_inode;

    uint32_t ioq_len=ioq_length(ioq);
    uint32_t size=ioq_len>count?count:ioq_len;
    while(bytes_read<size){
        *buffer=ioq_getchar(ioq);
        bytes_read++;
        buffer++;
    }
    return bytes_read;
}

uint32_t pipe_write(int32_t fd,const void* buf,uint32_t count){
    uint32_t bytes_write=0;
    int32_t global_fd=fd_local2global(fd);
    struct ioqueue* ioq=(struct ioqueue*)file_table[global_fd].fd_inode;

    uint32_t ioq_left=__bufsize-ioq_length(ioq);
    uint32_t size=ioq_left>count?count:ioq_left;

    const char* buffer=buf;
    while(bytes_write<size){
        ioq_putchar(ioq,*buffer);
        bytes_write++;
        buffer++;
    }
    return bytes_write;
}