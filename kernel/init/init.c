#include "../include/init.h"
#include "../include/print.h"
#include "../include/interrupt.h"
#include "../include/timer.h"
#include "../include/memory.h"
#include "../include/thread.h"
#include "../include/console.h"
#include "../include/keyboard.h"
#include "../include/process.h"

extern void tss_init();

void 
init_all    (void)
{
    put_string("Init all\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init();
    keyboard_init();
    console_init();
    //tss_init();
}
