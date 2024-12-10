#include "../include/stdint.h"
#include "../include/sync.h"
#include "../include/thread.h"
#include "../include/print.h"
#include "../include/fs.h"
#include "../include/log.h"
#include "../include/debug.h"
#include "../include/thread.h"
#include "../include/memory.h"
#include "../include/string.h"

bool log_write=False;

static struct lock log_lock;

void log_init(){
    lock_init(&log_lock);
    int32_t fd=-1;
    log_write=False;
    fd=sys_open(LOG_PROCESS_FILE,O_CREAT);
    sys_close(fd);
    fd=sys_open(LOG_ACESS_FILE,O_CREAT);
    sys_close(fd);
    fd=sys_open(LOG_SYSCALL_FILE,O_CREAT);
    sys_close(fd);
    fd=sys_open(LOG_DEVICE_FILE,O_CREAT);
    sys_close(fd);
}

void log_acquire(){
    lock_acquire(&log_lock);
}

void log_release(){
    lock_release(&log_lock);
}

static int32_t pos[20]={0};

static void process_log(char*str,char* filepath,enum log_type _log_type){
    int32_t fd=sys_open_log(filepath,O_RDWR);
    if(pos[_log_type]>=1024*60){
        pos[_log_type]=0;
    }
    
    if(pos[_log_type]==0){
        if(fd>=0){
            sys_remove_some_content(fd,0,1024*60);
        }
    }
    
    if(fd>=0){
        sys_write(fd,"[",pos[_log_type],1);
        pos[_log_type]++;
        sys_write(fd,running_thread()->name,pos[_log_type],strlen(running_thread()->name));
        pos[_log_type]+=strlen(running_thread()->name);
        sys_write(fd,"]",pos[_log_type],1);
        pos[_log_type]++;
        sys_write(fd,str,pos[_log_type],strlen(str));
        pos[_log_type]+=strlen(str);
    }
    sys_close(fd);
}

void log_put_string(char* str,enum log_type log_type){
    log_acquire();
    switch (log_type)
    {
        case PROCESS:
        {
            process_log(str,LOG_PROCESS_FILE,PROCESS);
            break;
        }
        case FILE:
        {
            process_log(str,LOG_ACESS_FILE,FILE);
            break;
        }
        case DEVICE:
        {
            process_log(str,LOG_DEVICE_FILE,DEVICE);
            break;
        }
        case SYSCALL:
        {
            process_log(str,LOG_SYSCALL_FILE,SYSCALL);
            break;
        }
    }
    log_release();
}

enum log_status logEnable(){
    log_write=True;
    return (enum log_status)log_write;
}

enum log_status logDisable(){
    log_write=False;
    return (enum log_status)log_write;
}

enum log_status log_setstatus(enum log_status status){
    ASSERT(status==log_disable||status==log_enable);
    enum log_status old_status=log_write;
    log_write=status;
    return old_status;
}

enum log_status get_log_status(){
    return (enum log_status)log_write;
}