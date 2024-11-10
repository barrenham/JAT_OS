#ifndef SYSCALL_H_
#define SYSCALL_H_
#include "stdint.h"
#include "fs.h"

#define syscall_nr      32
typedef void*           syscall;

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_MKDIR,
    SYS_OPENFILE,
    SYS_DELETE,
    SYS_READ,
    SYS_SEEK,
    SYS_RSC
};

uint32_t    sys_getpid(void);
uint32_t    getpid(void);
void        syscall_init(void);
int32_t     write(int32_t fd,const char*buf,uint32_t bufsize);
int32_t     sys_write_console(char* str);
void*       malloc(uint32_t size);
void        free(void* ptr);
int32_t     openFile(const char* pathname,uint8_t flags);
int32_t     mkdir(const char* pathname);
int32_t     read(int32_t fd,const char* buf,uint32_t bufsize);
int32_t     seekp(int32_t fd,int32_t offset,enum whence wh_type);
int32_t     remove_some_cotent(int32_t fd,int32_t size);

#endif