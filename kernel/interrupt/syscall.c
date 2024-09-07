#include "../include/syscall.h"
#include "../include/thread.h"
#include "../include/print.h"

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

void syscall_init(void){
    put_string("syscall_init start\n");
    syscall_table[SYS_GETPID]=sys_getpid;
    put_string("syscall_init done\n");
}

uint32_t getpid(){
    return  _syscall0(SYS_GETPID);
}