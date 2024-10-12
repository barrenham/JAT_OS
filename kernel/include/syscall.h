#ifndef SYSCALL_H_
#define SYSCALL_H_
#include "stdint.h"

#define syscall_nr      32
typedef void*           syscall;

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE
};

uint32_t    sys_getpid(void);
uint32_t    getpid(void);
void syscall_init(void);
uint32_t    write(char* str);
uint32_t    sys_write(char* str);
void*       malloc(uint32_t size);
void        free(void* ptr);


#endif