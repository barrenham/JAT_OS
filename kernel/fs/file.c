#include "../include/file.h"
#include "../include/stdint.h"
#include "../include/io.h"
#include "../include/debug.h"
#include "../include/global.h"
#include "../include/super_block.h"
#include "../include/dir.h"
#include "../include/inode.h"
#include "../include/fs.h"
#include "../include/ide.h"
#include "../include/stdio-kernel.h"
#include "../include/stdio.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/interrupt.h"

struct file file_table[MAX_FILE_OPEN];
extern struct partition* cur_part;

int32_t get_free_slot_in_global(void){
    uint32_t fd_idx=3;
    while(fd_idx<MAX_FILE_OPEN){
        if(file_table[fd_idx].fd_inode==NULL){
            break;
        }
        fd_idx++;
    }
    if(fd_idx==MAX_FILE_OPEN){
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}

int32_t pcb_fd_install(int32_t global_fd_idx){
    struct task_struct* cur=running_thread();
    uint8_t local_fd_idx=3;
    while(local_fd_idx<MAX_FILES_OPEN_PER_PROC){
        if(cur->fd_table[local_fd_idx]==-1){
            cur->fd_table[local_fd_idx]=global_fd_idx;
            break;
        }
        local_fd_idx++;
    }
    if(local_fd_idx==MAX_FILES_OPEN_PER_PROC){
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return local_fd_idx;
}

/**
 * @brief 从 inode 位图中分配一个 inode。
 *
 * 此函数扫描位图，查找一个未分配的 inode，并将其标记为已分配。(仅内存)
 * 
 * @param part 指向要操作的分区的指针。
 * @return 返回分配的 inode 索引，如果没有可用的 inode，则返回 -1。
 */
int32_t inode_bitmap_alloc(struct partition* part) {
    // 扫描位图，查找一个可用的 inode。
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1) {
        return -1; // 没有可用的 inode，返回 -1。
    }
    // 标记该 inode 为已分配。
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx; // 返回分配的 inode 索引。
}

int32_t inode_bitmap_dealloc(struct partition* part,uint32_t bit_idx) {
    if (bit_idx == -1) {
        return -1; 
    }
    // 标记该数据块为未分配。
    bitmap_set(&part->inode_bitmap, bit_idx, 0);
    // 返回取消分配的块的逻辑块地址。
    return (bit_idx);
}

/**
 * @brief 从块位图中分配一个数据块。
 *
 * 此函数扫描位图，查找一个未分配的数据块，并将其标记为已分配。（仅内存）
 *
 * @param part 指向要操作的分区的指针。
 * @return 返回分配的块的逻辑块地址(在硬盘的扇区号)，如果没有可用块，则返回 -1。
 */
int32_t block_bitmap_alloc(struct partition* part) {
    // 扫描位图，查找一个可用的数据块。
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1) {
        return -1; // 没有可用的块，返回 -1。
    }
    // 标记该数据块为已分配。
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    // 返回分配的块的逻辑块地址。
    return (part->sb->data_struct_lba + bit_idx);
}



int32_t block_bitmap_dealloc(struct partition* part,uint32_t bit_idx) {
    if (bit_idx == -1) {
        return -1; 
    }
    // 标记该数据块为未分配。
    bitmap_set(&part->block_bitmap, bit_idx, 0);
    // 返回取消分配的块的逻辑块地址。
    return (part->sb->data_struct_lba + bit_idx);
}

/**
 * @brief 同步位图到磁盘。
 *
 * 此函数将指定索引的位图写入磁盘，以保持位图的持久性。(同步硬盘)
 *
 * @param part 指向要操作的分区的指针。
 * @param bit_idx 要同步的位图索引。
 * @param btmp 指定的位图类型，可能为 inode_bitmap 或 block_bitmap。
 */
void bitmap_sync(struct partition* part, uint32_t bit_idx, enum bitmap_type btmp) {
    // 计算位图的偏移量和逻辑块地址。
    uint32_t off_sec = bit_idx / 4096; // 计算位图块的偏移（以块为单位）。
    uint32_t off_size = off_sec * BLOCK_SIZE; // 计算偏移字节。
    uint32_t sec_lba; // 逻辑块地址。
    uint8_t* bitmap_off; // 指向位图偏移的指针。

    // 根据位图类型设置逻辑块地址和位图偏移指针。
    switch (btmp) {
        case INODE_BITMAP:
            sec_lba = part->sb->inode_bitmap_lba + off_sec; // 获取 inode 位图的逻辑块地址。
            bitmap_off = part->inode_bitmap.bits + off_size; // 获取 inode 位图的偏移。
            break;
        case BLOCK_BITMAP:
            sec_lba = part->sb->block_bitmap_lba + off_sec; // 获取块位图的逻辑块地址。
            bitmap_off = part->block_bitmap.bits + off_size; // 获取块位图的偏移。
            break;
    }

    // 将位图写入磁盘。
    ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}

int32_t file_create(struct dir* parent_dir,char* filename,uint8_t flag)
{
    void* io_buf=sys_malloc(1024);
    if(io_buf==NULL){
        printk("in file_creat: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint8_t rollback_step=0;

    int32_t inode_no=inode_bitmap_alloc(cur_part);
    if(inode_no==-1){
        printk("in file_create: allocate inode failed\n");
        return -1;
    }
    struct inode* new_file_inode=(struct inode*)sys_malloc(sizeof(struct inode));
    if(new_file_inode==NULL){
        printk("file_create: sys_malloc for inode failed\n");
        rollback_step=1;
        goto rollback;
    }

    int fd_idx=get_free_slot_in_global();
    if(fd_idx==-1){
        printk("exceed max open files\n");
        rollback_step=2;
        goto rollback;
    }
    inode_init(inode_no,new_file_inode);

    file_table[fd_idx].fd_inode=new_file_inode;
    file_table[fd_idx].fd_pos=0;
    file_table[fd_idx].fd_inode->write_deny=False;
    file_table[fd_idx].fd_flag=flag;
    
    struct dir_entry new_dir_entry;
    memset(&new_dir_entry,0,sizeof(struct dir_entry));

    create_dir_entry(filename,inode_no,FT_REGULAR,&new_dir_entry);
    if(!sync_dir_entry(parent_dir,&new_dir_entry,io_buf)){
        printk("sync dir_entry to disk failed\n");
        rollback_step=3;
        goto rollback;
    }
    memset(io_buf,0,1024);
    inode_sync(cur_part,parent_dir->inode,io_buf);
    memset(io_buf,0,1024);
    inode_sync(cur_part,new_file_inode,io_buf);

    bitmap_sync(cur_part,inode_no,INODE_BITMAP);
    
    list_push(&cur_part->open_inodes,&new_file_inode->inode_tag);
    new_file_inode->i_open_cnts=1;

    sys_free(io_buf);

    return pcb_fd_install(fd_idx);

rollback:
    switch (rollback_step)
    {
        case 3:
        {
            memset(&file_table[fd_idx],0,sizeof(struct file));
        }
        case 2:
        {
            sys_free(new_file_inode);
        }
        case 1:
        {
            bitmap_set(&cur_part->inode_bitmap,inode_no,0);
            break;
        }
    }
    sys_free(io_buf);
    return -1;
}


/**
 * @brief 打开文件并返回文件描述符。
 *
 * 此函数根据给定的 inode 编号和打开标志打开文件，并返回一个文件描述符。
 * 如果文件描述符表已满，则返回 -1。
 *
 * @param inode_no 要打开的 inode 编号。
 * @param flag 文件打开标志，指示打开方式（只读、写入或读写）。
 * @return 文件描述符，如果失败则返回 -1。
 */
int32_t file_open(uint32_t inode_no, uint8_t flag) {
    // 获取一个空闲的文件描述符索引。
    int fd_idx = get_free_slot_in_global();
    if (fd_idx == -1) {
        printk("exceed max open files\n"); // 如果没有空闲的文件描述符，输出错误信息。
        return -1; // 返回 -1 表示失败。
    }
    
    // 打开 inode 并将其与文件描述符关联。
    file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
    file_table[fd_idx].fd_pos = 0; // 初始化文件位置为 0。
    file_table[fd_idx].fd_flag = flag; // 设置文件打开标志。

    bool* write_deny = &file_table[fd_idx].fd_inode->write_deny; // 获取写入拒绝标志的指针。

    // 检查文件打开模式，如果是只写或读写模式，则进行写入权限检查。
    if (flag & O_WRONLY || flag & O_RDWR) {
        enum intr_status old_status = intr_disable(); // 禁用中断以进行安全检查。
        
        if (!(*write_deny)) { // 如果当前没有写入拒绝。
            *write_deny = True; // 设置写入拒绝标志。
            intr_set_status(old_status); // 恢复中断状态。
        } else { // 如果当前文件不能被写入。
            intr_set_status(old_status); // 恢复中断状态。
            printk("file cannot be write now, try again later\n"); // 输出错误信息。
            return -1; // 返回 -1 表示失败。
        }
    }
    
    return pcb_fd_install(fd_idx); // 返回新安装的文件描述符。
}


/**
 * @brief 关闭文件并释放相关资源。
 *
 * 此函数关闭给定的文件，解除对其 inode 的写入拒绝状态，并释放 inode。
 *
 * @param file 指向要关闭的文件结构体的指针。
 * @return 返回 0 表示成功，返回 -1 表示失败（例如，传入的文件指针为 NULL）。
 */
int32_t file_close(struct file* file) {
    // 检查传入的文件指针是否为 NULL。
    if (file == NULL) {
        return -1; // 返回 -1 表示失败。
    }
    
    // 解除对 inode 的写入拒绝状态。
    file->fd_inode->write_deny = False;
    
    // 关闭 inode 并释放相关资源。
    inode_close(file->fd_inode);
    file->fd_inode=NULL;
    
    return 0; // 返回 0 表示成功。
}

int32_t file_write(struct file* File,const void* buf,uint32_t offset,uint32_t bufsize){
    bool isOutFileSize=File->fd_inode->i_size < offset;
    uint32_t secondary_lba=0;
    if((bufsize+offset)>(BLOCK_SIZE*140)){
        printk("exceed max file_size 71680 bytes , write file failed\n");
        return -1;
    }
    uint8_t* io_buf=sys_malloc(2*BLOCK_SIZE);
    if(io_buf==NULL){
        printk("file_write: sys_malloc for io_buf failed\n");
        sys_free(io_buf);
        return -1;
    }
    uint32_t all_blocks[140]={0};
    for(int i=0;i<13;i++){
        all_blocks[i]=File->fd_inode->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    uint32_t block_start=(offset/(BLOCK_SIZE));
    uint32_t in_block_offset=(offset%(BLOCK_SIZE));
    uint32_t bytes_passed=0;
    uint32_t blocks_passed=0;
    uint32_t needToWrite=bufsize;
    uint8_t* _buf=(uint8_t*)buf;

    while(blocks_passed<block_start){
        if(blocks_passed<12){
            if(all_blocks[blocks_passed]==0){
                int32_t block_idx=block_bitmap_alloc(cur_part);
                if(block_idx==-1){
                    printk("not enough block for data!\n");
                    sys_free(io_buf);
                    return -1;
                }
                all_blocks[blocks_passed]=File->fd_inode->i_sectors[blocks_passed]=block_idx;
                bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
            }else{
                ;
            }
        }else{
            if(blocks_passed==12){
                if(all_blocks[blocks_passed]==0){
                    int32_t block_idx=block_bitmap_alloc(cur_part);
                    int32_t secondary_block_idx=block_bitmap_alloc(cur_part);
                    if(block_idx==-1||secondary_block_idx==-1){
                        printk("not enough block for data!\n");
                        sys_free(io_buf);
                        return -1;
                    }
                    secondary_lba=block_idx;
                    memset(all_blocks+12,0,BLOCK_SIZE);
                    all_blocks[blocks_passed]=secondary_block_idx;
                    ide_write(cur_part->my_disk,secondary_block_idx,all_blocks+12,1);
                    File->fd_inode->i_sectors[blocks_passed]=block_idx;
                    bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                    bitmap_sync(cur_part,secondary_block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                }else{
                    ;
                }
            }else{
                if(all_blocks[blocks_passed]==0){
                    int32_t block_idx=block_bitmap_alloc(cur_part);
                    if(block_idx==-1){
                        printk("not enough block for data!\n");
                        sys_free(io_buf);
                        return -1;
                    }
                    all_blocks[blocks_passed]=block_idx;
                    ide_write(cur_part->my_disk,secondary_lba,all_blocks+12,1);
                    bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                }else{
                    ;
                }
            }
        }
        blocks_passed+=1;
    }

    //两种情况:
    //1. blocks_passed>12
    //2. blocks_passed==12
    //3. blocks_passed<12
    bytes_passed+=(blocks_passed*(BLOCK_SIZE));
    if(blocks_passed>12){
        if(all_blocks[blocks_passed]==0){
            int32_t block_idx=block_bitmap_alloc(cur_part);
            if(block_idx==-1){
                printk("not enough block for data!\n");
                sys_free(io_buf);
                return -1;
            }
            all_blocks[blocks_passed]=block_idx;
            ide_write(cur_part->my_disk,secondary_lba,all_blocks+12,1);
            bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
        }
    }else if(blocks_passed==12){
        if(all_blocks[blocks_passed]==0){
            int32_t block_idx=block_bitmap_alloc(cur_part);
            int32_t secondary_block_idx=block_bitmap_alloc(cur_part);
            if(block_idx==-1||secondary_block_idx==-1){
                printk("not enough block for data!\n");
                sys_free(io_buf);
                return -1;
            }
            secondary_lba=block_idx;
            memset(all_blocks+12,0,BLOCK_SIZE);
            all_blocks[blocks_passed]=secondary_block_idx;
            ide_write(cur_part->my_disk,secondary_block_idx,all_blocks+12,1);
            File->fd_inode->i_sectors[blocks_passed]=block_idx;
            bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
            bitmap_sync(cur_part,secondary_block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
        }
    }else{
        if(all_blocks[blocks_passed]==0){
            int32_t block_idx=block_bitmap_alloc(cur_part);
            if(block_idx==-1){
                printk("not enough block for data!\n");
                sys_free(io_buf);
                return -1;
            }
            all_blocks[blocks_passed]=File->fd_inode->i_sectors[blocks_passed]=block_idx;
            bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
        }
    }
    uint32_t writeSize=(needToWrite>=(BLOCK_SIZE-in_block_offset))?(BLOCK_SIZE-in_block_offset):(needToWrite);
    ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
    memcpy(io_buf+in_block_offset,_buf,writeSize);
    needToWrite-=writeSize;
    _buf+=writeSize;
    ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);

    blocks_passed+=1;
    bytes_passed+=writeSize;

    while(needToWrite>0){
        if(blocks_passed>12){
            if(all_blocks[blocks_passed]==0){
                int32_t block_idx=block_bitmap_alloc(cur_part);
                if(block_idx==-1){
                    printk("not enough block for data!\n");
                    sys_free(io_buf);
                    return -1;
                }
                all_blocks[blocks_passed]=block_idx;
                ide_write(cur_part->my_disk,secondary_lba,all_blocks+12,1);
                bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
            }
        }else if(blocks_passed==12){
            if(all_blocks[blocks_passed]==0){
                int32_t block_idx=block_bitmap_alloc(cur_part);
                int32_t secondary_block_idx=block_bitmap_alloc(cur_part);
                if(block_idx==-1||secondary_block_idx==-1){
                    printk("not enough block for data!\n");
                    sys_free(io_buf);
                    return -1;
                }
                secondary_lba=block_idx;
                memset(all_blocks+12,0,BLOCK_SIZE);
                all_blocks[blocks_passed]=secondary_block_idx;
                ide_write(cur_part->my_disk,secondary_block_idx,all_blocks+12,1);
                File->fd_inode->i_sectors[blocks_passed]=block_idx;
                bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                bitmap_sync(cur_part,secondary_block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
            }
        }else{
            if(all_blocks[blocks_passed]==0){
                int32_t block_idx=block_bitmap_alloc(cur_part);
                if(block_idx==-1){
                    printk("not enough block for data!\n");
                    sys_free(io_buf);
                    return -1;
                }
                all_blocks[blocks_passed]=File->fd_inode->i_sectors[blocks_passed]=block_idx;
                bitmap_sync(cur_part,block_idx-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
            }
        }
        uint32_t writeSize=(needToWrite>=BLOCK_SIZE)?(BLOCK_SIZE):(needToWrite);
        ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        memcpy(io_buf,_buf,writeSize);
        needToWrite-=writeSize;
        _buf+=writeSize;
        ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        blocks_passed+=1;
        bytes_passed+=writeSize;
    }

    File->fd_inode->i_size=((offset+bufsize)>=(File->fd_inode->i_size))?(offset+bufsize):(File->fd_inode->i_size);

    //同步inode到硬盘
    /*
    for(int i=0;i<13;i++){
        printk("%x ", File->fd_inode->i_sectors[i]);
    }
    */
    inode_sync(cur_part,File->fd_inode,io_buf);

    sys_free(io_buf);
    return (bytes_passed - offset);
}


int32_t file_read(struct file* File,const void* buf,uint32_t offset,uint32_t bufsize){
    if(offset>=File->fd_inode->i_size){
        return -1;
    }
    uint8_t* _buf=buf;
    uint32_t all_blocks[140]={0};
    uint32_t secondary_lba=0;
    for(int i=0;i<13;i++){
        all_blocks[i]=File->fd_inode->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }

    uint32_t blocks_passed=0;
    uint32_t in_block_offset=offset%(BLOCK_SIZE);
    uint32_t read_bytes=0;
    uint32_t left_to_read=(offset+bufsize>File->fd_inode->i_size)?(File->fd_inode->i_size-offset):(bufsize);
    blocks_passed=(offset)/(BLOCK_SIZE);
    if(all_blocks[blocks_passed]==0){
        printk("error occured when access to file inode-block 0x%x\n",blocks_passed);
        return -1;
    }

    uint8_t* io_buf=(uint8_t*)sys_malloc(BLOCK_SIZE*2);
    uint32_t read_in_block=((left_to_read)>=(BLOCK_SIZE-in_block_offset))?((BLOCK_SIZE-in_block_offset)):(left_to_read);
    ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
    memcpy(_buf,io_buf+in_block_offset,read_in_block);
    _buf+=read_in_block;
    left_to_read-=read_in_block;
    read_bytes+=read_in_block;
    blocks_passed+=1;
    while(left_to_read>0){
        if(all_blocks[blocks_passed]==0){
            printk("error occured when access to file inode-block 0x%x\n",blocks_passed);
            sys_free(io_buf);
            return -1;
        }
        uint32_t read_in_block=((left_to_read)>=(BLOCK_SIZE))?((BLOCK_SIZE)):(left_to_read);
        ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        memcpy(_buf,io_buf,read_in_block);
        _buf+=read_in_block;
        left_to_read-=read_in_block;
        read_bytes+=read_in_block;
        blocks_passed+=1;
    }
    
    sys_free(io_buf);
    return read_bytes;
}


int32_t file_remove_some_content(struct file* File,uint32_t offset,uint32_t size){
    ASSERT(File->fd_inode!=NULL);
    if(offset>=File->fd_inode->i_size){
        printk("remove exceed file size!\n");
        return -1;
    }
    
    bool should_delete_some_block=(offset+size) >= File->fd_inode->i_size;
    uint32_t all_blocks[140]={0};
    uint32_t secondary_lba=0;
    for(int i=0;i<13;i++){
        all_blocks[i]=File->fd_inode->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    uint32_t blocks_passed=offset/BLOCK_SIZE;
    uint32_t in_block_offset=offset%BLOCK_SIZE;
    uint8_t* io_buf=(uint8_t*)sys_malloc(2*BLOCK_SIZE);

rollback:
    if(should_delete_some_block){
        if(in_block_offset==0){
            if(blocks_passed<=12){
                while(all_blocks[blocks_passed]!=0){
                    block_bitmap_dealloc(cur_part,all_blocks[blocks_passed]-cur_part->sb->data_struct_lba);
                    bitmap_sync(cur_part,all_blocks[blocks_passed]-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                    all_blocks[blocks_passed]=0;
                    if(blocks_passed<13){
                        File->fd_inode->i_sectors[blocks_passed]=0;
                    }
                    blocks_passed+=1;
                }
                block_bitmap_dealloc(cur_part,secondary_lba-cur_part->sb->data_struct_lba);
                bitmap_sync(cur_part,secondary_lba-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                ASSERT(blocks_passed==DIV_ROUND_UP(File->fd_inode->i_size,BLOCK_SIZE));
            }else{
                while(all_blocks[blocks_passed]!=0){
                    block_bitmap_dealloc(cur_part,all_blocks[blocks_passed]-cur_part->sb->data_struct_lba);
                    bitmap_sync(cur_part,all_blocks[blocks_passed]-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
                    all_blocks[blocks_passed]=0;
                    blocks_passed+=1;
                }
                ide_read(cur_part->my_disk,secondary_lba,io_buf,1);
                memcpy(io_buf,all_blocks+12,BLOCK_SIZE);
                ide_write(cur_part->my_disk,secondary_lba,io_buf,1);
                ASSERT(blocks_passed==DIV_ROUND_UP(File->fd_inode->i_size,BLOCK_SIZE));
            }
            File->fd_inode->i_size=offset;
        }else{
            ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
            memset(io_buf+in_block_offset,0,BLOCK_SIZE-in_block_offset);
            ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
            blocks_passed+=1;
            in_block_offset=0;
            goto rollback;
        }
    }else{
        uint32_t left_to_remove=size;
        uint32_t remove_in_block=(left_to_remove>=(BLOCK_SIZE-in_block_offset))?\
                                 (BLOCK_SIZE-in_block_offset):(left_to_remove);
        ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        memset(io_buf+in_block_offset,0,remove_in_block);
        ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        left_to_remove-=remove_in_block;
        blocks_passed+=1;
        while(left_to_remove>0){
            uint32_t remove_in_block=(left_to_remove>=(BLOCK_SIZE))?\
                                 (BLOCK_SIZE):(left_to_remove);
            ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
            memset(io_buf,0,remove_in_block);
            ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
            left_to_remove-=remove_in_block;
            blocks_passed+=1;
        }
    }
    inode_sync(cur_part,File->fd_inode,io_buf);
    sys_free(io_buf);
    return File->fd_inode->i_size;
}