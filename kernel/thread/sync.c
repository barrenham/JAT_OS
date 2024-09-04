#include "../include/sync.h"
#include "../include/interrupt.h"
#include "../include/debug.h"

void sema_init(struct semaphore* psema,uint8_t value){
    psema->value=value;
    list_init(&psema->waiters);
}

void lock_init(struct lock* plock){
    plock->holder=NULL;
    plock->holder_repeat_nr=0;
    sema_init(&plock->semaphore,1);
}

void sema_down(struct semaphore* psema){
    enum intr_status old_status=intr_disable();
    while(psema->value==0){
        if(!elem_find(&psema->waiters,&running_thread()->general_tag))
            list_append(&psema->waiters,&running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    psema->value--;
    intr_set_status(old_status);
}

void sema_up(struct semaphore* psema){
    enum intr_status old_status=intr_disable();
    ASSERT(psema->value==0);
    if(!list_empty(&psema->waiters)){
        struct task_struct* thread_blocked=elem2entry(struct task_struct,general_tag,list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    intr_set_status(old_status);
}

void lock_acquire(struct lock* plock){
    enum intr_status old_status=intr_disable();
    if(plock->holder!=running_thread()){
        sema_down(&plock->semaphore);
        plock->holder=running_thread();
        ASSERT(plock->holder_repeat_nr==0);
        plock->holder_repeat_nr=1;
    }else{
        plock->holder_repeat_nr++;
    }
    intr_set_status(old_status);
}

void lock_release(struct lock* plock){
    enum intr_status old_status=intr_disable();
    ASSERT(plock->holder==running_thread());
    if(plock->holder_repeat_nr>1){
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr==1);
    plock->holder=NULL;
    plock->holder_repeat_nr=0;
    sema_up(&plock->semaphore);
    intr_set_status(old_status);
}
