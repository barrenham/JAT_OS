#include "../include/syscall.h"
#include "../include/thread.h"
#include "../include/console.h"
#include "../include/string.h"
#include "../include/print.h"
#include "../include/memory.h"
#include "../include/fs.h"
#include "../include/fault.h"
#include "../include/debug.h"
#include "../include/process.h"
#include "../include/pipe.h"
#include "../include/exec.h"

typedef int16_t  pid_t;

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

int32_t delete(const char* pathname){
    return _syscall1(SYS_DELETE,pathname);
}

int32_t closeFile(int32_t fd){
    return _syscall1(SYS_CLOSEFILE,fd);
}

int32_t copyFile(const char* dst_path,const char* src_path){
    return user_file_copy(dst_path,src_path);
}

filesize getfilesize(file_descriptor fd){
    return _syscall1(SYS_GETFILESIZE,fd);
}

int32_t user_file_copy(const char* dst_path,const char* src_path){
    int fds=openFile(src_path,O_RDONLY);
    if(fds<0){
        return GENERAL_FAULT;
    }
    int fdd=openFile(dst_path,O_WRONLY);
    if(fdd<0){
        closeFile(fds);
        return GENERAL_FAULT;
    }
    char* buf=malloc(512);
    if(buf==NULL){
        closeFile(fds);
        closeFile(fdd);
        free(buf);
        return GENERAL_FAULT;
    }
    int32_t read_cnt=0;
    while((read_cnt=read(fds,buf,512))>=0){
        write(fdd,buf,read_cnt);
    }
    closeFile(fds);
    closeFile(fdd);
    free(buf);
    return 0;
}


int32_t process_exit(void){
    return _syscall0(EXIT_PROCESS);
}

int32_t fork(void){
    int32_t cpid=_syscall0(SYS_FORK);
    return cpid;
}

enum file_types get_file_type(const char* filepath){
    return _syscall1(SYS_GETFILEATTRIBUTE,filepath);
}

int32_t pipe(int fd[2]){
    return _syscall1(SYS_PIPE,fd);
}

int32_t execv(const char* filepath,char** argv){
    return _syscall2(SYS_EXEC,filepath,argv);
}

char getchar(){
    return _syscall0(SYS_GETCHAR);
}