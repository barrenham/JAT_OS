#ifndef SYSCALL_H_
#define SYSCALL_H_
#include "stdint.h"

#define syscall_nr      32
typedef void*           syscall;

enum SYSCALL_NR{
    SYS_GETPID
};

uint32_t    sys_getpid(void);
uint32_t    getpid(void);
void syscall_init(void);

#endif