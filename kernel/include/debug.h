#ifndef __LIB_KERNEL_DEBUG_H
#define __LIB_KERNEL_DEBUG_H

void 
panic_spin(char*filename,int line, const char*func,const char* condition);

#define PANIC(...) panic_spin(__FILE__,__LINE__,__func__,__VA_ARGS__)

#ifdef  NDEBUG
    #define ASSERT(condition) ((void)0)
#else
    #define ASSERT(condition) \
    if(condition){}else{  \
        PANIC(#condition);\
    } 
#endif  /*  NDEBUG  */

#define TEST_PRINT


#endif  /*  DEBUG_H  */