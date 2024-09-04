#ifndef __KERNEL_KEYBOARD_H
#define __KERNEL_KEYBOARD_H

#include "ioqueue.h"
extern struct ioqueue kbd_buf;

void keyboard_init();

#endif