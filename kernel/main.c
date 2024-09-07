#include "include/print.h"
#include "include/console.h"
#include "include/init.h"
#include "include/debug.h"
#include "include/string.h"
#include "include/memory.h"
#include "include/thread.h"
#include "include/interrupt.h"
#include "include/console.h"
#include "include/ioqueue.h"
#include "include/keyboard.h"
#include "include/process.h"
#include "include/syscall.h"

int a=0,b=0;
void test_thread1(void* arg);
void test_thread2(void* arg);
void u_prog_a(void);
void u_prog_b(void);

int main(void) {
   put_string("I am kernel\n");
   init_all();
   
   thread_start("kernel_thread_a",31,test_thread1," thread_A:0x");
   thread_start("kernel_thread_b",31,test_thread2," thread_B:0x");
   process_execute(u_prog_a,"user_prog_a");
   process_execute(u_prog_b,"user_prog_b"); 
   intr_enable();
   
   console_put_string(" main_pid:0x");
   console_put_int(sys_getpid());
   console_put_char('\n');

   while(1);
   return 0;
}

void test_thread1(void* arg)
{
    while(1){
    console_put_string((char*)arg);
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_string(" u_prog_a:0x");
    console_put_int(a);
    console_put_char('\n');
    }
    while(1);
}

void test_thread2(void* arg)
{
    while(1){
    console_put_string((char*)arg);
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_string(" u_prog_b:0x");
    console_put_int(b);
    console_put_char('\n');
    }
    while(1);
}

void u_prog_a(void)
{
    a = getpid();
    while(1);
}

void u_prog_b(void)
{
    b = getpid();
    while(1);
}
