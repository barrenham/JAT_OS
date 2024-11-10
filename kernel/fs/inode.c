#include "../include/stdint.h"
#include "../include/io.h"
#include "../include/debug.h"
#include "../include/global.h"
#include "../include/interrupt.h"
#include "../include/super_block.h"
#include "../include/dir.h"
#include "../include/inode.h"
#include "../include/fs.h"
#include "../include/ide.h"
#include "../include/stdio-kernel.h"
#include "../include/stdio.h"
#include "../include/memory.h"
#include "../include/string.h"

struct inode_position{
    bool two_sec;
    uint32_t sec_lba;
    uint32_t off_size;
};

extern struct file file_table[MAX_FILE_OPEN];

/**
 * @brief 定位指定分区中的 inode。
 *
 * 此函数根据 inode 编号计算该 inode 在 inode 表中的位置。
 *
 * @param part 指向分区结构体的指针。
 * @param inode_no 要定位的 inode 编号。
 * @param inode_pos 指向 inode 位置结构体的指针，用于返回定位结果。
 */
static void inode_locate(struct partition* part,
                         uint32_t inode_no,
                         struct inode_position* inode_pos) 
{
    ASSERT(inode_no < 4096); // 确保 inode 编号小于 4096。
    
    uint32_t inode_table_lba = part->sb->inode_table_lba; // 获取 inode 表的 LBA 地址。
    uint32_t inode_size = sizeof(struct inode); // 获取 inode 的大小。
    uint32_t off_size = inode_no * inode_size; // 计算 inode 的偏移量。
    
    uint32_t off_sec = off_size / SECTOR_SIZE; // 计算偏移的扇区数。
    uint32_t off_size_in_sec = off_size % SECTOR_SIZE; // 计算在扇区内的偏移量。

    // 检查 inode 是否跨越两个扇区。
    bool left_in_sec = ((off_size_in_sec + inode_size) > SECTOR_SIZE) ? (True) : (False);
    inode_pos->two_sec = left_in_sec; // 设置是否跨扇区的标志。
    inode_pos->sec_lba = inode_table_lba + off_sec; // 设置 inode 所在扇区的 LBA 地址。
    inode_pos->off_size = off_size_in_sec; // 设置在扇区内的偏移量。
}

/**
 * @brief 将 inode 同步到磁盘。
 *
 * 此函数将内存中的 inode 数据写入磁盘，以保持数据的一致性。
 *
 * @param part 指向分区结构体的指针。
 * @param inode 指向要同步的 inode 结构体的指针。
 * @param io_buf 用于 I/O 操作的缓冲区指针。
 */
void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {
    uint8_t inode_no = inode->i_no; // 获取 inode 编号。
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos); // 定位 inode。

    struct inode pure_inode; 
    memcpy(&pure_inode, inode, sizeof(struct inode)); // 复制 inode 数据到临时变量。

    // 清除不必要的字段以避免写入脏数据。
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = False;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

    char* inode_buf = (char*)io_buf; // 将 I/O 缓冲区转换为字符指针。
    if (inode_pos.two_sec) { // 如果 inode 跨越两个扇区。
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2); // 从磁盘读取两个扇区。
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode)); // 更新 inode 数据。
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2); // 将更新后的数据写回磁盘。
    } else { // 如果 inode 仅占用一个扇区。
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1); // 从磁盘读取一个扇区。
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode)); // 更新 inode 数据。
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1); // 将更新后的数据写回磁盘。
    }
}



/**
 * @brief 打开指定分区中的 inode。
 *
 * 此函数通过遍历打开的 inode 列表检查该 inode 是否已被打开。
 * 如果找到，则增加打开计数并返回该 inode。
 * 如果未找到，则定位 inode 在磁盘上的位置，读取 inode 数据到内存中，然后返回新打开的 inode。
 * (该注释由ChatGPT生成)
 * 
 * @param part 指向要打开 inode 的分区结构体的指针。
 * @param inode_no 要打开的 inode 编号。
 * @return 指向打开的 inode 结构体的指针，如果 inode 无法打开则返回 NULL。
 */
struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    // 遍历打开的 inode 列表，检查该 inode 是否已被打开。
    struct list_elem* elem = part->open_inodes.head.next;
    struct inode* inode_found;

    while (elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        
        // 如果找到 inode，增加打开计数并返回。
        if (inode_found->i_no == inode_no) {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next; // 移动到列表中的下一个元素。
    }

    // 如果未找到 inode，定位它在磁盘上的位置。
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);

    // 备份当前的页目录，并将其设置为 NULL 以便进行内存分配。
    struct task_struct* cur = running_thread();
    uint32_t* cur_pagedir_bak = cur->pgdir;
    cur->pgdir = NULL;

    // 为 inode 结构体分配内存。
    inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
    cur->pgdir = cur_pagedir_bak; // 恢复原始的页目录。

    // 分配一个缓冲区以从磁盘读取 inode 数据。
    char* inode_buf;
    if (inode_pos.two_sec) {
        inode_buf = (char*)sys_malloc(1024); // 如果需要，分配 2 个扇区的缓冲区。
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
        inode_buf = (char*)sys_malloc(512); // 否则分配 1 个扇区的缓冲区。
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }

    // 将缓冲区中的 inode 数据复制到 inode 结构体中。
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    // 将新打开的 inode 添加到打开的 inode 列表中。
    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1; // 初始化打开计数为 1。

    // 释放用于读取 inode 数据的缓冲区。
    sys_free(inode_buf);
    
    return inode_found; // 返回打开的 inode。
}


/**
 * @brief 关闭指定的 inode。
 *
 * 此函数减少 inode 的打开计数。如果打开计数减少到 0，
 * 则将该 inode 从打开的 inode 列表中移除，并释放其内存。
 *
 * @param inode 指向要关闭的 inode 结构体的指针。
 */
void inode_close(struct inode* inode) {
    // 禁用中断并备份当前中断状态。
    enum intr_status old_status = intr_disable();
    if(inode->i_no==0){
        intr_set_status(old_status);
        return;
    }
    // 减少打开计数，如果计数减至 0。
    if (--inode->i_open_cnts == 0) {
        // 从打开的 inode 列表中移除该 inode。
        list_remove(&inode->inode_tag);
        
        // 获取当前正在运行的线程。
        struct task_struct* cur = running_thread();
        uint32_t* cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL; // 将页目录设置为 NULL 以进行内存释放。

        // 释放 inode 的内存。
        sys_free(inode);
        
        // 恢复原始的页目录。
        cur->pgdir = cur_pagedir_bak;
    }
    
    // 恢复之前的中断状态。
    intr_set_status(old_status);
}


void inode_init(uint32_t inode_no,struct inode* new_inode){
    new_inode->i_no=inode_no;
    new_inode->i_size=0;
    new_inode->i_open_cnts=0;
    new_inode->write_deny=False;

    uint8_t sec_idx=0;
    while(sec_idx<13){
        new_inode->i_sectors[sec_idx]=0;
        sec_idx++;
    }
}

void inode_delete(struct partition* part,uint32_t inode_no,void* io_buf){
    ASSERT(inode_no<4096);
    struct inode_position inode_pos;
    inode_locate(part,inode_no,&inode_pos);

    char* inode_buf=(char*)io_buf;
    if(inode_pos.two_sec){
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,2);
        memset((inode_buf+inode_pos.off_size),0,sizeof(struct inode));
        ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,2);
    }else{
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,1);
        memset((inode_buf+inode_pos.off_size),0,sizeof(struct inode));
        ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,1);
    }
    inode_bitmap_dealloc(part,inode_no);
    bitmap_sync(part,inode_no,INODE_BITMAP);
}