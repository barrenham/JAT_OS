#include "debug.h"
#include "print.h"
#include "interrupt.h"



void 
panic_spin(char* filename,
           int line,
           const char* func,
           const char* condition)
{
    intr_disable();

    put_string("\n\n\n!!!!! error !!!!!\n");
    put_string("filename:");
    put_string(filename);
    put_string("\n");
    put_string("condition:");
    put_string((char*)condition);
    put_string("\nline:");
    put_int(line);
    put_string("\n");
    while(1);
}