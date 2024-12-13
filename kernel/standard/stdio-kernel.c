#include "../include/stdio-kernel.h"
#include "../include/log.h"

uint32_t printk(const char* format,...){
    va_list args;
    va_start(args,format);
    char buf[128];
    vsprintf(buf,format,args);
    va_end(args);
    if(get_log_status()){
        log_put_string(buf,PROCESS);
    }
}

uint32_t log_printk(enum log_type _log_type,const char* format,...){
    va_list args;
    va_start(args,format);
    char buf[128];
    vsprintf(buf,format,args);
    va_end(args);
    if(get_log_status()){
        log_put_string(buf,_log_type);
    }
}