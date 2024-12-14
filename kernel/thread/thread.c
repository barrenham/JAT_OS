#include "thread.h"
#include "../include/thread.h"
#include "../include/string.h"
#include "../include/print.h"
#include "global.h"
#include "../include/memory.h"
#include "../include/interrupt.h"
#include "../include/debug.h"
#include "../include/process.h"
#include "../include/sync.h"
#include "../include/userprog.h"
#include "../include/fs.h"
#include "../include/file.h"

extern struct file file_table[MAX_FILE_OPEN];
extern struct pool kernel_pool,user_pool;
extern struct virtual_addr kernel_vaddr;

struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
struct list thread_ready_second_list;
struct list thread_ready_third_list;

struct lock pid_lock;
static struct list_elem* thread_tag;
struct task_struct* idle_thread;


extern void switch_to(struct task_struct* cur,struct task_struct* next);

struct task_struct* running_thread(){
    uint32_t esp;
    asm volatile("mov %%esp,%0":"=g"(esp));
    return (struct task_struct*)(esp&0xfffff000);
}

static void idle(void* arg UNUSED){
    while(1){
        thread_block(TASK_WAITING);
        asm volatile("sti;hlt": : :"memory");
    }
}

pid_t allocate_pid(void){
    static pid_t next_pid=0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

static void kernel_thread(thread_func* function,void* func_arg){
    intr_enable();
    function(func_arg);
    thread_exit();
}

void 
thread_create(struct task_struct*pthread,
              thread_func function,
              void* func_arg)
{
    pthread->self_kstack-=sizeof(struct intr_stack);
    pthread->self_kstack-=sizeof(struct thread_stack);
    struct thread_stack* kthread_stack=(struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip=kernel_thread;
    kthread_stack->function=function;
    kthread_stack->func_arg=func_arg;
    kthread_stack->ebp=kthread_stack->ebx= \
    kthread_stack->esi=kthread_stack->edi= 0;
}

void
init_thread(struct task_struct* pthread,
            char*name,
            int prio)
{
    memset(pthread,0,sizeof(*pthread));
    pthread->pid=allocate_pid();
    pthread->parent_pid=running_thread()->pid;
    strcpy(pthread->name,name);
    if(pthread==main_thread){
        pthread->status=TASK_RUNNING;
    }else{
         pthread->status=TASK_READY;
    }
    pthread->ticks=prio;
    pthread->elapsed_ticks=0;
    pthread->pgdir=NULL;
    struct virtual_addr tmp={0};
    pthread->userprog_vaddr=tmp;
    pthread->priority=prio;
    pthread->self_kstack=(uint32_t*)((uint32_t)pthread+PG_SIZE);
    pthread->fd_table[0]=0;
    pthread->fd_table[1]=1;
    pthread->fd_table[2]=2;
    uint8_t fd_idx=3;
    while(fd_idx<MAX_FILES_OPEN_PER_PROC){
        pthread->fd_table[fd_idx]=-1;
        fd_idx++;
    }
    sema_init(&pthread->waiting_sema,0);
    pthread->stack_magic=0x20240825;
}

struct task_struct* thread_start(char* name,
                                 int prio,
                                 thread_func function,
                                 void* func_arg)
{
    enum intr_status old_status=intr_disable();
    struct task_struct* thread=get_kernel_pages(1);
    init_thread(thread,name,prio);
    thread_create(thread,function,func_arg);
    ASSERT(!elem_find(&thread_all_list,&thread->all_list_tag));
    list_append(&thread_all_list,&thread->all_list_tag);
    switch(prio){
        
        case FIRST_PRIO:
        {
            ASSERT(!elem_find(&thread_ready_list,&thread->general_tag));
            list_append(&thread_ready_list,&thread->general_tag);
            break;
        }
        case SECOND_PRIO:
        {
            ASSERT(!elem_find(&thread_ready_second_list,&thread->general_tag));
            list_append(&thread_ready_second_list,&thread->general_tag);
            break;
        }
        case THIRD_PRIO:
        {
            ASSERT(!elem_find(&thread_ready_third_list,&thread->general_tag));
            list_append(&thread_ready_third_list,&thread->general_tag);
            break;
        }
        default:
        {
            ASSERT(!elem_find(&thread_ready_third_list,&thread->general_tag));
            list_append(&thread_ready_third_list,&thread->general_tag);
            break;
        }
    }
    
    intr_set_status(old_status);
    return thread;
}

static void make_main_thread(void){
    main_thread=running_thread();
    init_thread(main_thread,"main",FIRST_PRIO);

    ASSERT(!elem_find(&thread_all_list,&main_thread->all_list_tag));
    list_append(&thread_all_list,&main_thread->all_list_tag);
}

void schedule(){
    ASSERT(intr_get_status()==INTR_OFF);
    
    struct task_struct* cur=running_thread();
    //list_remove(&cur->general_tag);
    if(list_empty(&thread_ready_list)&&list_empty(&thread_ready_second_list)&&list_empty(&thread_ready_third_list)){
        thread_unblock_idle(idle_thread);
    }
    thread_tag=NULL;
    /*
    struct list_elem* elem=thread_ready_second_list.head.next;
    while(elem->next!=NULL){
        struct task_struct* pcb=elem2entry(struct task_struct,general_tag,elem);
        put_string(pcb->name);put_string("->");
        elem=elem->next;
    }
    printf("\n");
    */
    
    if(!list_empty(&thread_ready_list)){
        thread_tag=list_pop(&thread_ready_list);
    }else if(!list_empty(&thread_ready_second_list)){
        thread_tag=list_pop(&thread_ready_second_list);
    }else{
        thread_tag=list_pop(&thread_ready_third_list);
    }
    struct task_struct*next=elem2entry(struct task_struct,general_tag,thread_tag);
    
    if(cur->status==TASK_RUNNING){
        /*
        ASSERT(!elem_find(&thread_ready_list,&cur->general_tag));
        list_append(&thread_ready_list,&cur->general_tag);
        cur->status=TASK_READY;
        cur->ticks=cur->priority;
        */
        if(cur->ticks==__UINT32_MAX__){
            cur->status=TASK_READY;
            switch(cur->priority){
                case FIRST_PRIO:
                {
                    cur->priority=SECOND_PRIO;
                    cur->ticks=SECOND_TICKS;
                    ASSERT(!elem_find(&thread_ready_second_list,&cur->general_tag));
                    list_append(&thread_ready_second_list,&cur->general_tag);
                    break;
                }
                case SECOND_PRIO:
                {
                    cur->priority=THIRD_PRIO;
                    cur->ticks=THIRD_TICKS;
                    ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
                    list_append(&thread_ready_third_list,&cur->general_tag);
                    break;
                }
                case THIRD_PRIO:
                {
                    cur->priority=THIRD_PRIO;
                    cur->ticks=THIRD_TICKS;
                    ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
                    list_append(&thread_ready_third_list,&cur->general_tag);
                    break;
                }
                default:
                {
                    cur->priority=THIRD_PRIO;
                    cur->ticks=THIRD_TICKS;
                    ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
                    list_append(&thread_ready_third_list,&cur->general_tag);
                    break;
                }
            }
        }else{
            cur->status=TASK_READY;
            switch(cur->priority){
                case FIRST_PRIO:
                {
                    cur->priority=FIRST_PRIO;
                    ASSERT(!elem_find(&thread_ready_list,&cur->general_tag));
                    list_append(&thread_ready_list,&cur->general_tag);
                    break;
                }
                case SECOND_PRIO:
                {
                    cur->priority=SECOND_PRIO;
                    ASSERT(!elem_find(&thread_ready_second_list,&cur->general_tag));
                    list_append(&thread_ready_second_list,&cur->general_tag);
                    break;
                }
                case THIRD_PRIO:
                {
                    cur->priority=THIRD_PRIO;
                    ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
                    list_append(&thread_ready_third_list,&cur->general_tag);
                    break;
                }
                default:
                {
                    cur->priority=THIRD_PRIO;
                    ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
                    list_append(&thread_ready_third_list,&cur->general_tag);
                    break;
                }
            }
        }
    }else{
        if(cur->status==TASK_BLOCKED){
            //list_remove(&cur->general_tag);
            cur->priority=THIRD_PRIO;
            //ASSERT(!elem_find(&thread_ready_third_list,&cur->general_tag));
            //list_append(&thread_ready_third_list,&cur->general_tag);
        }
        if(cur->status==TASK_DIED){
            list_remove(&cur->general_tag);
            if(cur->pgdir!=NULL){
                struct virtual_addr* user_vaddr=&cur->userprog_vaddr;
                uint32_t  vaddr=(void*)USER_VADDR_START;
                while((uint32_t)vaddr<(USER_STACK3_VADDR)+PG_SIZE){
                    if(bitmap_scan_test(&user_vaddr->vaddr_bitmap,((uint32_t)vaddr-USER_VADDR_START)/PG_SIZE)==1){
                        //put_int(vaddr);
                        uint32_t phyaddr=addr_v2p(vaddr);
                        //put_string("->");
                        //put_int(phyaddr);
                        if(phyaddr!=0){
                            bitmap_set(get_phy_bitmap_ptr(&user_pool),(phyaddr-get_phy_addr_start(&user_pool))/PG_SIZE,0);
                        }
                    }
                    vaddr+=0x1000;
                }
            }
        }
        
    }
    next->status=TASK_RUNNING;
    //ASSERT(next!=cur);
    
     //put_string(cur->name);
     //put_string("->");
     //put_string(next->name);
    

    process_activate(next);
    asm volatile ("pusha");
    switch_to(cur,next);
    asm volatile ("popa");
}

void thread_block(enum task_status stat){
    enum intr_status old_status=intr_disable();
    ASSERT((stat==TASK_BLOCKED)||(stat==TASK_WAITING)||(stat==TASK_HANGING));
    struct task_struct* cur_thread=running_thread();
    cur_thread->status=stat;
    schedule();
    intr_set_status(old_status);
}

void thread_unblock_idle(struct task_struct* pthread){
    enum intr_status old_status=intr_disable();

    //ASSERT((pthread->status==TASK_BLOCKED)||(pthread->status==TASK_WAITING)||(pthread->status==TASK_HANGING));
    if(pthread->status!=TASK_READY){
        ASSERT(!elem_find(&thread_ready_third_list,&pthread->general_tag));
        list_push(&thread_ready_third_list,&pthread->general_tag);
        pthread->status=TASK_READY;
    }
    intr_set_status(old_status);
}

void thread_unblock(struct task_struct* pthread){
    enum intr_status old_status=intr_disable();
    ASSERT((pthread->status==TASK_BLOCKED)||(pthread->status==TASK_WAITING)||(pthread->status==TASK_HANGING));
    if(pthread->status!=TASK_READY){
        list_remove(&pthread->general_tag);
        pthread->priority=FIRST_PRIO;
        pthread->ticks=FIRST_TICKS;
        list_push(&thread_ready_list,&pthread->general_tag);
        pthread->status=TASK_READY;
    }
    intr_set_status(old_status);
}

void thread_init(void){
    put_string("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    list_init(&thread_ready_second_list);
    list_init(&thread_ready_third_list);
    lock_init(&pid_lock);
    make_main_thread();
    idle_thread=thread_start("idle",THIRD_PRIO,idle,NULL);
    ASSERT(idle_thread!=NULL);
    put_string("thread_init done\n");
}

void thread_yield(void){
    struct task_struct* cur=running_thread();
    enum intr_status old_status=intr_disable();
    ASSERT(!elem_find(&thread_ready_list,&cur->general_tag));
    list_append(&thread_ready_list,&cur->general_tag);
    cur->priority=FIRST_PRIO;
    cur->status=TASK_READY;
    schedule();
    intr_set_status(old_status);
}

void thread_wait(void){
    sema_down(&(running_thread()->waiting_sema));
}

void thread_exit(void){
    struct task_struct* cur=running_thread();
    enum intr_status old_status=intr_disable();
    list_remove(&cur->general_tag);
    cur->status=TASK_DIED;
    if(cur->parent_pid!=-1){
        struct list_elem* elem=thread_all_list.head.next;
        while(elem->next!=NULL){
            struct task_struct* pcb=elem2entry(struct task_struct,all_list_tag,elem);
            if(pcb->pid==cur->parent_pid){
                sema_up(&(pcb->waiting_sema));
            }
            elem=elem->next;
        }
    }
    schedule();
    intr_set_status(old_status);
}

void thread_cleaner(void* args){
    while(1){
        struct list_elem* elem=thread_all_list.head.next;
        while(elem->next!=NULL){
            struct list_elem* next_elem=elem->next;
            struct task_struct* pcb=elem2entry(struct task_struct,all_list_tag,elem);
            if(pcb->status==TASK_DIED){
                //clean filetable
                printk("cleaning: %s\n",pcb->name);
                for(file_descriptor fd=3;fd<MAX_FILES_OPEN_PER_PROC;fd++){
                    if(pcb->fd_table[fd]!=-1){
                        file_descriptor _fd=fd_local2global(fd);
                        if(_fd>=0){
                            file_close(&file_table[_fd]);
                        }
                    }
                }
                //clean stack
                enum intr_status old_status=intr_disable();
                list_remove(&pcb->all_list_tag);
                list_remove(&pcb->general_tag);
                intr_set_status(old_status);
                if((pcb->pgdir)!=NULL){
                    mfree_page(PF_KERNEL,pcb->pgdir,1);
                    mfree_page(PF_KERNEL,pcb->userprog_vaddr.vaddr_bitmap.bits,DIV_ROUND_UP(\
                            pcb->userprog_vaddr.vaddr_bitmap.btmp_bytes_len,\
                            PG_SIZE));
                }
                mfree_page(PF_KERNEL,pcb,1);
                printk("cleaned: %s\n",pcb->name);
            }
            elem=next_elem;
        }

        //thread_block(TASK_WAITING);
    }
}