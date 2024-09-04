#ifndef __KERNEL_PROCESS_H
#define __KERNEL_PROCESS_H

#define USER_VADDR_START 0x8048000

#include "stdint.h"

void update_tss_esp(struct task_struct* pthread);

void tss_init();

void 
start_process(void* filename_);

void page_dir_activate(struct task_struct* p_thread);

void process_activate(struct task_struct* p_thread);

uint32_t* create_page_dir(void);

void create_user_vaddr_bitmap(struct task_struct* user_prog);

void process_execute(void* filename,char* name);

#endif