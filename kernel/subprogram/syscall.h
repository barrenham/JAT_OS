#ifndef SYSCALL_H
#define SYSCALL_H
#include "../include/stdint.h"
#include "../include/fs.h"
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
int32_t     delete(const char* pathname);
int32_t     user_file_copy(const char* dst_path,const char* src_path);
int32_t     copyFile(const char* dst_path,const char* src_path);
int32_t     closeFile(int32_t fd);
int32_t     getfilesize(int32_t fd);
int32_t     process_exit(void);
int32_t     fork(void);
enum file_types 
            get_file_type(const char* filepath);
int32_t     pipe(int fd[2]);

#endif