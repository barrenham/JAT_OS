#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H
#include "stdint.h"

#define PIC_M_CTRL          0x20
#define PIC_M_DATA          0x21
#define PIC_S_CTRL          0xa0
#define PIC_S_DATA          0xa1

#define EFLAGS_IF           0x00000200 //eflags寄存器if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0":"=g"(EFLAG_VAR))

void intr_exit(void);

enum intr_status{
    INTR_OFF,
    INTR_ON
};

enum    intr_status intr_get_status(void);
enum    intr_status intr_set_status(enum intr_status);
enum    intr_status intr_enable(void);
enum    intr_status intr_disable(void);

typedef void* intr_handler;
void idt_init(void);
void schedule();
void 
register_handler(uint8_t vector_no,
                 intr_handler function);

#endif