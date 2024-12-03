#include "../include/stdio-kernel.h"

uint32_t printk(const char* format,...){
    va_list args;
    va_start(args,format);
    char buf[1024];
    vsprintf(buf,format,args);
    va_end(args);
    console_put_string(buf);
}