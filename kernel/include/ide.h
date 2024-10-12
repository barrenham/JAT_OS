#ifndef IDE_H_
#define IDE_H_
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"
#include "bool.h"



struct partition{
    uint32_t start_lba;
    uint32_t sec_cnt;
    struct disk* my_disk;
    struct list_elem part_tag;
    char name[8];
    struct super_block* sb;
    struct bitmap block_bitmap;
    struct bitmap inode_bitmap;
    struct list open_inodes;
};

struct partition_table_entry{
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sec;
    uint8_t start_chs;
    uint8_t fs_type;
    uint8_t end_head;
    uint8_t end_sec;
    uint8_t end_chs;
    uint32_t start_lba;
    uint32_t sec_cnt;
} __attribute__ ((packed));

struct disk{
    char name[8];
    struct ide_channel* my_channel;
    uint8_t dev_no;
    struct partition prim_parts[4];
    struct partition logic_parts[8];
};



struct boot_sector{
    uint8_t other[446];
    struct partition_table_entry partition_table[4];
    uint16_t signature;
};


struct ide_channel{
    char name[8];
    uint16_t port_base;
    uint8_t irq_no;
    struct lock lock;
    bool expecting_intr;
    struct semaphore disk_done;
    struct disk devices[2];
};

void ide_init();
void intr_hd_handler(uint8_t irq_no);
void ide_read(struct disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt);
void ide_write(struct disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt);


#endif