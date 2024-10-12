#include "../include/syscall.h"
#include "../include/thread.h"
#include "../include/console.h"
#include "../include/string.h"
#include "../include/print.h"
#include "../include/memory.h"

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

#define _syscall3(NUMBER,ARG1,ARG2,ARG3) ({ \
    int retval;                             \
    asm volatile(                           \
        "int $0x80"                         \
        :"=a"(retval)                       \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3)  \
        :"memory");                         \
    retval;                                 \
})

syscall syscall_table[syscall_nr];

uint32_t    sys_getpid(void){
    return running_thread()->pid;
}

uint32_t    sys_write(char* str){
    console_put_string(str);
    return strlen(str);
}

void syscall_init(void){
    put_string("syscall_init start\n");
    syscall_table[SYS_GETPID]=sys_getpid;
    syscall_table[SYS_WRITE]=sys_write;
    syscall_table[SYS_MALLOC]=sys_malloc;
    syscall_table[SYS_FREE]=sys_free;
    put_string("syscall_init done\n");
}

uint32_t getpid(){
    return  _syscall0(SYS_GETPID);
}

uint32_t write(char* str){
    return  _syscall1(SYS_WRITE,str);
}

void* malloc(uint32_t size){
    return (void*)_syscall1(SYS_MALLOC,size);
}

void free(void* ptr){
    _syscall1(SYS_FREE,ptr);
}