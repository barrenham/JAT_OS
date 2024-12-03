#include "../include/ioqueue.h"
#include "../include/interrupt.h"
#include "../include/global.h"
#include "../include/debug.h"
#include "../include/bool.h"

void ioqueue_init(struct ioqueue* ioq){
    lock_init(&ioq->lock_producer);
    lock_init(&ioq->lock_consumer);
    ioq->producer=ioq->consumer=NULL;
    ioq->head=ioq->tail=0;
}

static int32_t next_pos(int32_t pos){
    return (pos+1)%__bufsize;
}

bool ioq_full(struct ioqueue*ioq){
    //ASSERT(intr_get_status()==INTR_OFF);
    return next_pos(ioq->head)==ioq->tail;
}

bool ioq_empty(struct ioqueue*ioq){
    //ASSERT(intr_get_status()==INTR_OFF);
    return ioq->head==ioq->tail;
}

static void ioq_wait(struct task_struct** waiter){
    ASSERT(*waiter==NULL&&waiter!=NULL);
    *waiter=running_thread();
    thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct** waiter){
    ASSERT(*waiter!=NULL);
    thread_unblock(*waiter);
    *waiter=NULL;
}

char ioq_getchar(struct ioqueue* ioq){
    //ASSERT(intr_get_status()==INTR_OFF);
    while (ioq_empty(ioq))
    {
        lock_acquire(&ioq->lock_consumer);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock_consumer);
    }
    char byte=ioq->buf[ioq->tail];
    ioq->tail=next_pos(ioq->tail);

    if(ioq->producer!=NULL){
        wakeup(&ioq->producer);
    }
    return byte;
    
}

void ioq_putchar(struct ioqueue* ioq,char byte){
    //ASSERT(intr_get_status()==INTR_OFF);
    while(ioq_full(ioq)){
        lock_acquire(&ioq->lock_producer);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock_producer);
    }
    ioq->buf[ioq->head]=byte;
    ioq->head=next_pos(ioq->head);
    if(ioq->consumer!=NULL){
        wakeup(&ioq->consumer);
    }
}

uint32_t ioq_length(struct ioqueue* ioq){
    uint32_t len=0;
    if(ioq->head>=ioq->tail){
        len=ioq->head-ioq->tail;
    }else{
        len=__bufsize-(ioq->tail-ioq->head);
    }
    return len;
}