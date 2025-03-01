#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H

#include "list.h"
#include "stdint.h"
#include "bitmap.h"
#include "thread.h"

#ifndef __SEMA_THREAD_SYNC_H
#define __SEMA_THREAD_SYNC_H
    struct semaphore{
        uint8_t value;
        struct list waiters;
    };
#endif


struct lock{
    struct task_struct* holder;
    struct semaphore semaphore;
    uint32_t holder_repeat_nr;
};



void sema_init(struct semaphore* psema,uint8_t value);

void lock_init(struct lock* plock);

void sema_down(struct semaphore* psema);

void sema_up(struct semaphore* psema);

void lock_acquire(struct lock* plock);

void lock_release(struct lock* plock);



#endif