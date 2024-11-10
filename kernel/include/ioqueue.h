#ifndef __KERNEL_IOQUEUE_H
#define __KERNEL_IOQUEUE_H

#include "stdint.h"
#include "thread.h"
#include "sync.h"
#include "bool.h"

#define __bufsize         64

struct ioqueue{
    struct lock lock_consumer;
    struct lock lock_producer;
    struct task_struct* producer;
    struct task_struct* consumer;
    char buf[__bufsize];
    int32_t head;
    int32_t tail;
};

bool ioq_empty(struct ioqueue*ioq);
bool ioq_full(struct ioqueue*ioq);
void ioqueue_init(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq,char byte);

#endif