#include "../include/console.h"
#include "../include/interrupt.h"
#include "../include/thread.h"
#include "../include/process.h"
#include "../include/print.h"
#include "../include/stdio.h"
#include "../include/syscall.h"
#include "../include/string.h"
#include "../include/fs.h"


static void itoa(uint32_t value,char** buf_ptr_addr,uint8_t base){
    uint32_t m=value%base;
    uint32_t i=value/base;
    if(i){
        itoa(i,buf_ptr_addr,base);
    }
    if(m<10){
        *((*buf_ptr_addr)++)=m+'0';
    }else{
        *((*buf_ptr_addr)++)=m-10+'A';
    }
}

uint32_t vsprintf(char* str,const char* format,va_list ap){
    char* buf_ptr=str;
    const char* index_ptr=format;
    char index_char=*index_ptr;
    char* arg_str;
    int32_t arg_int;
    char arg_char;
    while(index_char){
        if(index_char!='%'){
            *(buf_ptr++)=index_char;
            index_char=*(++index_ptr);
            continue;
        }
        index_char=*(++index_ptr);
        int padding = 0;
        if (index_char == '0')
        {
            index_char = *(++index_ptr);
            while (index_char >= '0' && index_char <= '9')
            {
                padding = padding * 10 + (index_char - '0');
                index_char = *(++index_ptr);
            }
        }
        switch(index_char){
            case 'x':
                arg_int=va_arg(ap,int);
                // itoa(arg_int,&buf_ptr,16);
                // index_char=*(++index_ptr);
                // break;
                if (padding > 0)
                {
                    char tmp[32] = {0};
                    char *tmp_ptr = tmp;
                    itoa(arg_int, &tmp_ptr, 16);
                    int len = strlen(tmp);
                    while (len < padding)
                    {
                        *(buf_ptr++) = '0';
                        len++;
                    }
                    strcpy(buf_ptr, tmp);
                    buf_ptr += strlen(tmp);
                }
                else
                    itoa(arg_int, &buf_ptr, 16);
                index_char = *(++index_ptr);
                break;
            case 's':
                arg_str=va_arg(ap,char*);
                strcpy(buf_ptr,arg_str);
                buf_ptr+=strlen(arg_str);
                index_char=*(++index_ptr);
                break;
            case 'c':
                arg_char=va_arg(ap,char);
                *(buf_ptr++)=arg_char;
                index_char=*(++index_ptr);
                break;
            case 'd':
                arg_int=va_arg(ap,int);
                itoa(arg_int,&buf_ptr,10);
                index_char=*(++index_ptr);
                break;
        }
    }
    *(buf_ptr++)='\0';
    return strlen(str);
}

uint32_t printf(const char* format,...){
    va_list args;
    va_start(args,format);
    char buf[256]={0};
    vsprintf(buf,format,args);
    va_end(args);
    return write(stdout_no,buf,strlen(buf));
}

uint32_t sprintf(char* buf,const char* format,...){
    va_list args;
    va_start(args,format);
    vsprintf(buf,format,args);
    va_end(args);
    return 0;
}

int32_t putchar(int32_t c){
    char buf[2]={0};
    buf[0]=c;
    return write(stdout_no,buf,1);
}

int32_t puts(const char* s){
    char buf[256]={0};
    strcpy(buf,s);
    return write(stdout_no,buf,strlen(buf));
}