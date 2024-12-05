#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
void        put_char(uint8_t char_asci);
void        put_string(char* message);
void        put_int(int number);
void        set_cursor(int position);
void        clean_screen(void);
#endif  