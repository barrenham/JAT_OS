#ifndef __LIB_KERNEL_STDIO_KERNEL_H
#define __LIB_KERNEL_STDIO_KERNEL_H
#include "stdio.h"
#include "stdint.h"
#include "console.h"
#include "log.h"

uint32_t printk(const char* format,...);

uint32_t log_printk(enum log_type _log_type,const char* format,...);

#endif