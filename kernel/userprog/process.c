#include "../include/global.h"
#include "../include/thread.h"
#include "../include/io.h"
#include "../include/print.h"
#include "../include/interrupt.h"
#include "../include/memory.h"
#include "../include/userprog.h"
#include "../include/process.h"
#include "../include/string.h"
#include "../include/console.h"
#include "../include/debug.h"
#include "../include/list.h"
#include "../include/stdio.h"
#include "../include/file.h"
#include "../include/inode.h"
#include "../include/pipe.h"
#include "../include/log.h"

extern void intr_exit(void);
extern struct task_struct* main_thread;
extern struct list thread_ready_list;
extern struct list thread_all_list;
extern struct list thread_ready_second_list;
extern struct list thread_ready_third_list;
extern struct file file_table[MAX_FILE_OPEN];

void
fork_start_process(void* filename_,void* eip,void* esp){
    #ifndef TEST_PRINT
        put_string(filename_);
    #endif
    void* function=filename_;
    struct task_struct* cur=running_thread();
    cur->self_kstack+=sizeof(struct thread_stack);
    struct intr_stack* proc_stack=(struct intr_stack*)cur->self_kstack;
    void* tmp=get_a_page(PF_USER,USER_STACK3_VADDR);
    proc_stack->edi=proc_stack->esi=proc_stack->ebp=proc_stack->esp_dummy=0;
    proc_stack->ebx=proc_stack->edx=proc_stack->ecx=proc_stack->eax=0;
    proc_stack->gs=0;
    proc_stack->fs=0;
    proc_stack->ds=proc_stack->es=SELECTOR_U_DATA;
    proc_stack->eip=eip;
    proc_stack->cs=SELECTOR_U_CODE;
    proc_stack->eflags=(EFLAGS_IOPL_0|EFLAGS_MBS|EFLAGS_IF_1);
    proc_stack->esp=(void*)((uint32_t)(tmp)+PG_SIZE);
    proc_stack->ss=SELECTOR_U_DATA;
    asm volatile("movl %0,%%esp;"
                : 
                :"g"(proc_stack)
                :"memory");
    asm volatile("jmp intr_exit");
}

void 
start_process(void* filename_){
    #ifndef TEST_PRINT
        put_string(filename_);
    #endif
    intr_disable();
    void* function=filename_;
    struct task_struct* cur=running_thread();
    cur->self_kstack+=sizeof(struct thread_stack);
    struct intr_stack* proc_stack=(struct intr_stack*)cur->self_kstack;
    void* tmp=get_a_page(PF_USER,USER_STACK3_VADDR);
    proc_stack->edi=proc_stack->esi=proc_stack->ebp=proc_stack->esp_dummy=0;
    proc_stack->ebx=proc_stack->edx=proc_stack->ecx=proc_stack->eax=0;
    proc_stack->gs=0;
    proc_stack->fs=0;
    proc_stack->ds=proc_stack->es=SELECTOR_U_DATA;
    proc_stack->eip=function;
    proc_stack->cs=SELECTOR_U_CODE;
    proc_stack->eflags=(EFLAGS_IOPL_0|EFLAGS_MBS|EFLAGS_IF_1);
    proc_stack->esp=(void*)((uint32_t)(tmp)+PG_SIZE);
    proc_stack->ss=SELECTOR_U_DATA;
    intr_enable();
    asm volatile("movl %0,%%esp;"
                : 
                :"g"(proc_stack)
                :"memory");
    asm volatile("jmp intr_exit");
}

void page_dir_activate(struct task_struct* p_thread){
    uint32_t pagedir_phy_addr=KERNEL_PAGE_DIR;
    // put_int(p_thread->pgdir);
    if(p_thread->pgdir!=NULL){
        pagedir_phy_addr=addr_v2p((uint32_t)p_thread->pgdir);
        // put_int(pagedir_phy_addr);
        // for (int i = 0; i < 1024; i++)
        //     put_int(*((uint32_t*)pagedir_phy_addr + i));
    }
    // put_int(pagedir_phy_addr);
    
    // put_int(pagedir_phy_addr);
    asm volatile("movl %0,%%cr3"
                :
                :"r"(pagedir_phy_addr)
                :"memory");
    // if (p_thread->pgdir != NULL)
    // {
    //     put_int(pagedir_phy_addr);
    //     while (1);
    // }
}

void process_activate(struct task_struct* p_thread){
    ASSERT(p_thread!=NULL);
    page_dir_activate(p_thread);
    //put_string("\npage_dir_activate finish\n");
    if(p_thread->pgdir){
        update_tss_esp(p_thread);
    }
}

uint32_t* create_page_dir(void){
    uint32_t* page_dir_vaddr=get_kernel_pages(1);
    if(page_dir_vaddr==NULL){
        console_put_string("create_page_dir: get_kernel_page failed!\n");
        return NULL;
    }
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr),\
          (uint32_t*)(0xfffff000),\
          4);
    // put_int((uint32_t)*page_dir_vaddr);
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr+0x300*4),\
          (uint32_t*)(0xfffff000+0x300*4),\
          1024);
    // put_int((uint32_t)page_dir_vaddr);
    uint32_t new_page_dir_phy_addr=addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[1023]=new_page_dir_phy_addr|PG_US_U|PG_RW_W|PG_P_1;
    #ifndef TEST_PRINT
    for(int i=0;i<1024;i++){
        put_int(page_dir_vaddr[i]);
        put_char(' ');
    }
    #endif
    //while(1);
    return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct* user_prog){
    user_prog->userprog_vaddr.vaddr_start=USER_VADDR_START;
    uint32_t bitmap_pg_cnt=DIV_ROUND_UP(\
                            (0xc0000000-USER_VADDR_START)/PG_SIZE/8,\
                            PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits=\
    get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len=(0xc0000000-USER_VADDR_START)/PG_SIZE/8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}
struct task_struct* fork_process_execute(void* filename,char* name){
    enum intr_status old_status=intr_disable();
    struct task_struct* thread=get_kernel_pages(1);
    init_thread(thread,name,default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread,start_process,filename);
    thread->pgdir=create_page_dir();
    block_desc_init(thread->u_block_desc);
    intr_set_status(old_status);
    return thread;
}

void process_execute(void* filename,char* name){
    enum intr_status old_status=intr_disable();
    struct task_struct* thread=get_kernel_pages(1);
    init_thread(thread,name,default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread,start_process,filename);
    thread->pgdir=create_page_dir();
    // for (int i = 0; i < 1024; i++)
    //     put_int(thread->pgdir[i]);
    block_desc_init(thread->u_block_desc);
    //put_int(&thread_ready_list);
    struct list_elem* elem=thread_ready_list.head.next;
    ASSERT(!elem_find(&thread_ready_second_list,&thread->general_tag));
    list_append(&thread_ready_second_list,&thread->general_tag);
    ASSERT(!elem_find(&thread_all_list,&thread->all_list_tag));
    list_append(&thread_all_list,&thread->all_list_tag);
    intr_set_status(old_status);
}

void exit_process(void){
    thread_exit();
}

static int32_t copy_pcb_stack0_and_vaddrBitmap(struct task_struct* child_thread,struct task_struct* parent_thread){
    memcpy(child_thread,parent_thread,PG_SIZE);
    child_thread->pid=allocate_pid();
    child_thread->parent_pid=parent_thread->pid;
    child_thread->priority=THIRD_PRIO;
    child_thread->ticks=THIRD_TICKS;
    child_thread->general_tag.prev=child_thread->general_tag.next=NULL;
    child_thread->all_list_tag.prev=child_thread->all_list_tag.next=NULL;
    uint32_t bitmap_pg_cnt=DIV_ROUND_UP(\
                            child_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len,\
                            PG_SIZE);
    child_thread->userprog_vaddr.vaddr_bitmap.bits=get_kernel_pages(bitmap_pg_cnt);
    child_thread->userprog_vaddr.vaddr_start=parent_thread->userprog_vaddr.vaddr_start;
    bitmap_init(&child_thread->userprog_vaddr.vaddr_bitmap);
    memcpy(child_thread->userprog_vaddr.vaddr_bitmap.bits,\
           parent_thread->userprog_vaddr.vaddr_bitmap.bits,\
           child_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len);
    char buf[20]={0};
    sprintf(buf,"f_%s",parent_thread->name);
    ASSERT(buf[19]=='\0');
    strcpy(child_thread->name,buf);
    return 0;
}

static int32_t copy_page_table(struct task_struct* child_thread,struct task_struct* parent_thread){
    child_thread->pgdir=create_page_dir();
    if(child_thread->pgdir==NULL){
        return -1;
    }
    uint8_t* vaddr_bitmap=parent_thread->userprog_vaddr.vaddr_bitmap.bits;
    uint32_t btmp_bytes_len=parent_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len;
    uint32_t vaddr_start=parent_thread->userprog_vaddr.vaddr_start;
    uint32_t idx_byte=0;
    uint32_t idx_bit=0;
    uint32_t prog_vaddr=0;
    uint8_t* buf=get_kernel_pages(1);
    if(buf==NULL){
        return -1;
    }
    while(idx_byte<btmp_bytes_len){
        if(vaddr_bitmap[idx_byte]){
            idx_bit=0;
            while(idx_bit<8){
                if((BITMAP_MASK<<idx_bit)&vaddr_bitmap[idx_byte]){
                    prog_vaddr=(idx_byte*8+idx_bit)*PG_SIZE+vaddr_start;
                    memcpy(buf,prog_vaddr,PG_SIZE);
                    page_dir_activate(child_thread);
                    get_a_page_without_opvaddrbitmap(PF_USER,prog_vaddr);
                    memcpy(prog_vaddr,buf,PG_SIZE);
                    page_dir_activate(parent_thread);
                }
                idx_bit++;
            }
        }
        idx_byte++;
    }
    page_dir_activate(parent_thread);
    mfree_page(PF_KERNEL,buf,1);
    return 0;
}

static int32_t build_child_thread(struct task_struct* child_thread){
    struct intr_stack* stack0=(uint32_t)child_thread+PG_SIZE-sizeof(struct intr_stack);
    stack0->eax=0;
    uint32_t* ret_addr_in_thread_stack=(uint32_t*)stack0-1;
    uint32_t* esi_ptr_in_thread_stack= (uint32_t*)stack0-2;
    uint32_t* edi_ptr_in_thread_stack = (uint32_t*)stack0-3;
    uint32_t* ebx_ptr_in_thread_stack = (uint32_t*)stack0-4;
    uint32_t* ebp_ptr_in_thread_stack = (uint32_t*)stack0-5;
    *ret_addr_in_thread_stack=(uint32_t)intr_exit;
    *esi_ptr_in_thread_stack=*edi_ptr_in_thread_stack=*ebx_ptr_in_thread_stack=\
    *ebp_ptr_in_thread_stack=0;
    child_thread->self_kstack=ebp_ptr_in_thread_stack;
    return 0;
}

static void update_inode_open_cnts(struct task_struct* thread){
    int32_t local_fd=3,global_fd=0;
    while(local_fd<MAX_FILES_OPEN_PER_PROC){
        global_fd=thread->fd_table[local_fd];
        if(global_fd>=3&&!is_pipe(local_fd)){
            file_table[global_fd].fd_inode->i_open_cnts++;
        }
        local_fd++;
    }
}

pid_t sys_fork(void){
    enum intr_status old_status=intr_disable();
    struct task_struct* cur=running_thread();
    struct task_struct* child_thread=get_kernel_pages(1);
    copy_pcb_stack0_and_vaddrBitmap(child_thread,cur);
    copy_page_table(child_thread,cur);
    build_child_thread(child_thread);
    update_inode_open_cnts(child_thread);
    ASSERT(!elem_find(&thread_ready_third_list,&child_thread->general_tag));
    list_append(&thread_ready_third_list,&child_thread->general_tag);
    ASSERT(!elem_find(&thread_all_list,&child_thread->all_list_tag));
    list_append(&thread_all_list,&child_thread->all_list_tag);
    sema_init(&(child_thread->waiting_sema),0);
    log_printk(PROCESS,"%s forking a new process %s\n",cur->name,child_thread->name);
    //intr_set_status(old_status);
    return child_thread->pid;
}