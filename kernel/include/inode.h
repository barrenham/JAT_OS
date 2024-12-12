#ifndef __INODE_H
#define __INODE_H

#include "stdint.h"
#include "bool.h"
#include "list.h"
#include "ide.h"

#define INODE_WRITE_DENY 0x1
#define INODE_ENCRYPTED 0x2

struct inode{
    uint32_t i_no;
    uint32_t i_size;
    uint32_t i_open_cnts;
    // bool write_deny;
    int flags;

    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};

void inode_sync(struct partition* part,struct inode* inode,void* io_buf);

void inode_delete(struct partition* part,uint32_t inode_no,void* io_buf);

struct inode* inode_open(struct partition* part,uint32_t inode_no);

void inode_close(struct inode* inode);

void inode_init(uint32_t inode_no,struct inode* new_inode);

#endif