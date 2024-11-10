#include "../include/syscall.h"
#include "../include/thread.h"
#include "../include/console.h"
#include "../include/string.h"
#include "../include/print.h"
#include "../include/memory.h"
#include "../include/fs.h"

#define _syscall0(NUMBER) ({    \
    int retval;                 \
    asm volatile(               \
        "int $0x80"             \
        :"=a"(retval)           \
        :"a"(NUMBER)            \
        :"memory"               \
    );                          \
    retval;                     \
})

#define _syscall1(NUMBER,ARG1) ({    \
    int retval;                 \
    asm volatile(               \
        "int $0x80"             \
        :"=a"(retval)           \
        :"a"(NUMBER),"b"(ARG1)            \
        :"memory"               \
    );                          \
    retval;                     \
})

#define _syscall2(NUMBER,ARG1,ARG2) ({    \
    int retval;                 \
    asm volatile(               \
        "int $0x80"             \
        :"=a"(retval)           \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2)            \
        :"memory"               \
    );                          \
    retval;                     \
})

#define _syscall3(NUMBER,ARG1,ARG2,ARG3) ({ \
    int retval;                             \
    asm volatile(                           \
        "int $0x80"                         \
        :"=a"(retval)                       \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3)  \
        :"memory");                         \
    retval;                                 \
})

#define _syscall4(NUMBER,ARG1,ARG2,ARG_PTR_3_4) ({ \
    int retval;                             \
    asm volatile(                           \
        "int $0x80"                         \
        :"=a"(retval)                       \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG_PTR_3_4)  \
        :"memory");                         \
    retval;                                 \
})

syscall syscall_table[syscall_nr];

uint32_t    sys_getpid(void){
    return running_thread()->pid;
}

int32_t    sys_write_console(char* str){
    console_put_string(str);
    return strlen(str);
}


void syscall_init(void){
    put_string("syscall_init start\n");
    syscall_table[SYS_GETPID]=sys_getpid;
    syscall_table[SYS_WRITE]=user_file_write;
    syscall_table[SYS_MALLOC]=sys_malloc;
    syscall_table[SYS_FREE]=sys_free;
    syscall_table[SYS_OPENFILE]=user_file_open;
    syscall_table[SYS_MKDIR]=user_mkdir;
    syscall_table[SYS_READ]=user_file_read;
    syscall_table[SYS_SEEK]=user_file_seekp;
    syscall_table[SYS_RSC]=user_file_remove_some_content;
    put_string("syscall_init done\n");
}

uint32_t getpid(){
    return  _syscall0(SYS_GETPID);
}

int32_t write(int32_t fd,const char*buf,uint32_t bufsize){
    return  _syscall3(SYS_WRITE,fd,buf,bufsize);
}

void* malloc(uint32_t size){
    return (void*)_syscall1(SYS_MALLOC,size);
}

void free(void* ptr){
    _syscall1(SYS_FREE,ptr);
}

int32_t openFile(const char* pathname,uint8_t flags){
    return _syscall2(SYS_OPENFILE,pathname,flags);
}

int32_t mkdir(const char* pathname){
    return _syscall1(SYS_MKDIR,pathname);
}

int32_t read(int32_t fd,const char* buf,uint32_t bufsize){
    return _syscall3(SYS_READ,fd,buf,bufsize);
}

int32_t seekp(int32_t fd,int32_t offset,enum whence wh_type){
    return _syscall3(SYS_SEEK,fd,offset,wh_type);
}

int32_t remove_some_cotent(int32_t fd,int32_t size){
    return _syscall2(SYS_RSC,fd,size);
}