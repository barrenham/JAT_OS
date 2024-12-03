#ifndef __LIB__STDIO_H
#define __LIB__STDIO_H
#include "stdint.h"


typedef void* va_list;

#define     va_start(ap,v)  ap=(va_list)&v
#define     va_arg(ap,t)    *((t*)(ap+=4))
#define     va_end(ap)      ap=NULL

uint32_t vsprintf(char* str,const char* format,va_list ap);
uint32_t printf(const char* format, ...);
uint32_t sprintf(char* const buf,const char* format,...);
int32_t putchar(int32_t c);
int32_t puts(const char* s);

#endif
