#ifndef TIMER_H
#define TIMER_H

#define IRQ0_FREQUENCY      100
#define INPUT_FREQUENCY     1193180
#define COUNTER0_VALUE      INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT       0x40
#define COUNTER0_NO         0
#define COUNTER_MODE        2
#define READ_WRITE_LATCH    3
#define PIT_CONTROL_PORT    0x43

#include "stdint.h"
#include "interrupt.h"
void timer_init();

#endif