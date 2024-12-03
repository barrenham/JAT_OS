#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"

#define PG_SIZE 4096
#define default_prio 20  //second prio == process default_prio
#define MAX_FILES_OPEN_PER_PROC 8

#define FIRST_PRIO 30
#define SECOND_PRIO 20
#define THIRD_PRIO 10

#define FIRST_TICKS 25
#define SECOND_TICKS 15
#define THIRD_TICKS  10

typedef void thread_func(void*);
typedef int16_t  pid_t;

enum task_status{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

struct intr_stack{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;

};

struct thread_stack{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip)(thread_func*func,void* func_arg);
    void (*unused_retaddr);
    thread_func* function;
    void* func_arg;
};

#ifndef __SEMA_THREAD_SYNC_H
#define __SEMA_THREAD_SYNC_H
    struct semaphore{
        uint8_t value;
        struct list waiters;
    };
#endif

struct task_struct{
    uint32_t*               self_kstack;
    pid_t                   pid;
    pid_t                   parent_pid;
    enum    task_status     status;
    uint8_t                 priority;
    uint32_t                ticks;
    uint32_t                elapsed_ticks;
    struct list_elem        general_tag;
    struct list_elem        all_list_tag;
    char                    name[16];
    uint32_t*               pgdir;
    struct virtual_addr     userprog_vaddr;
    struct mem_block_desc   u_block_desc[DESC_CNT];
    int32_t                 fd_table[MAX_FILES_OPEN_PER_PROC];
    struct semaphore        waiting_sema;
    uint32_t                stack_magic;
};

extern struct task_struct* thread_start(char* name,
                                    int prio,
                                    thread_func function,
                                    void* func_arg);

struct task_struct* running_thread();
void thread_init(void);

void thread_block(enum task_status stat);

void thread_unblock(struct task_struct* pthread);

void thread_cleaner(void);

void 
thread_create(struct task_struct*pthread,
              thread_func function,
              void* func_arg);

void
init_thread(struct task_struct* pthread,
            char*name,
            int prio);

struct task_struct* thread_start(char* name,
                                 int prio,
                                 thread_func function,
                                 void* func_arg);

void thread_unblock_idle(struct task_struct* pthread);

void thread_yield(void);

void thread_exit(void);

pid_t allocate_pid(void);

void thread_wait(void);

#endif