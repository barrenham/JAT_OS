#include "../include/stdint.h"
#include "../include/sync.h"
#include "../include/thread.h"
#include "../include/print.h"

static struct lock console_lock;

void console_init(){
    lock_init(&console_lock);
}

void console_acquire(){
    lock_acquire(&console_lock);
}

void console_release(){
    lock_release(&console_lock);
}

void console_put_string(char* str){
    console_acquire();
    put_string(str);
    console_release();
}

void console_put_char(uint8_t char_asci){
    console_acquire();
    put_char(char_asci);
    console_release();
}

void console_put_int(uint32_t num){
    console_acquire();
    put_int(num);
    console_release();
}

void console_clear(void)
{
    for (int i = 0; i < 80 * 25; i++)
        console_put_char(' ');
    set_cursor(0);
}
