#ifndef __LIB_KERNEL_MEMORY_H
#define __LIB_KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

enum pool_flags
{
    PF_KERNEL=1,
    PF_USER=2
};


struct virtual_addr{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

extern struct pool kernel_pool,user_pool;
void mem_init(void);

void* get_kernel_pages(uint32_t pg_cnt);

void* malloc_page(enum pool_flags pf,uint32_t pg_cnt);

void* get_user_pages(uint32_t pg_cnt);

void* get_a_page(int pf,uint32_t vaddr);

uint32_t addr_v2p(uint32_t vaddr);



#define PG_P_1          1
#define PG_P_0          0
#define PG_RW_R         0
#define PG_RW_W         2
#define PG_US_S         0
#define PG_US_U         4

#endif