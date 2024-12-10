#ifndef FILE_H
#define FILE_H
#include"stdint.h"
#include "ide.h"

struct file{
    uint32_t fd_pos;
    uint8_t  fd_flag;
    struct inode* fd_inode;
};

extern struct dir;

enum std_fd{
    stdin_no,
    stdout_no,
    stderr_no
};

enum fd_flag{
    NONE,
    PIPE_FLAG
};

enum bitmap_type{
    INODE_BITMAP,
    BLOCK_BITMAP,
};

#define MAX_FILE_OPEN 32

int32_t get_free_slot_in_global(void);

int32_t pcb_fd_install(int32_t global_fd_idx);

int32_t inode_bitmap_alloc(struct partition* part);

int32_t block_bitmap_alloc(struct partition* part);

int32_t block_bitmap_dealloc(struct partition* part,uint32_t bit_idx);

int32_t inode_bitmap_dealloc(struct partition* part,uint32_t bit_idx);

void bitmap_sync(struct partition* part,uint32_t bit_idx,enum bitmap_type btmp);

int32_t file_create(struct dir* parent_dir,char* filename,uint8_t flag);

int32_t file_open(uint32_t inode_no,uint8_t flag);

int32_t file_close(struct file* file);


/**
 * @brief 将数据写入指定偏移位置的文件。
 *
 * 此函数处理文件的写入，通过管理块的分配来确保数据正确写入。
 * 它支持直接和间接块管理，适用于超过 12 块的文件。
 *
 * @param File 指向表示要写入的文件的文件结构体的指针。
 * @param buf 指向包含要写入数据的缓冲区的指针。
 * @param offset 写入操作开始的位置。
 * @param bufsize 要从缓冲区写入的字节数。
 * @return 写入的字节数，成功返回写入字节数，失败返回 -1。
 *
 * 函数执行以下步骤：
 * 1. 检查请求的写入是否会超过最大文件大小。
 * 2. 分配一个用于 I/O 操作的缓冲区。
 * 3. 从文件的 inode 中读取现有块信息。
 * 4. 根据需要预分配块，直到指定的偏移位置。
 * 5. 将数据写入相应的块，处理块内偏移。
 * 6. 继续写入，直到所有数据写入完毕或块用尽。
 * 7. 更新 inode 中的文件大小，并将 inode 同步到磁盘。
 *
 * 此函数还包括内存分配和块分配失败的错误处理，确保在返回错误代码之前 
 * 正确清理资源。
 */
int32_t file_write(struct file* File,const void* buf,uint32_t offset,uint32_t bufsize);

/**
 * 从指定文件中读取数据。
 * 
 * @param File 文件指针，指向要读取的文件结构。
 * @param buf 指向缓冲区的指针，用于存放读取的数据。
 * @param offset 从文件中的哪个位置开始读取数据。
 * @param bufsize 要读取的字节数。
 * @return 实际读取的字节数，如果发生错误则返回 -1。
 *
 * 函数的工作流程如下：
 * 1. 检查请求读取的字节数是否超出文件的实际大小。如果超出，打印错误信息并返回 -1。
 * 2. 初始化局部变量，获取文件的所有数据块信息。
 * 3. 如果文件的第一个块不存在，打印错误信息并返回 -1。
 * 4. 为读取数据分配缓冲区，读取当前数据块的数据。
 * 5. 从读取的缓冲区中将所需的数据复制到用户提供的缓冲区。
 * 6. 进入循环，继续读取后续的数据块，直到完成指定的读取字节数。
 * 7. 每次读取后更新相关的指针和计数器。
 * 8. 释放分配的缓冲区并返回实际读取的字节数。
 */
int32_t file_read(struct file* File,const void* buf,uint32_t offset,uint32_t bufsize);


/**
 * 从指定文件中删除某些内容。
 * 
 * @param File 文件指针，指向要修改的文件结构。
 * @param offset 从文件中的哪个位置开始删除内容。
 * @param size 要删除的字节数。
 * @return 文件的新大小，如果发生错误则返回 -1。
 *
 * 函数的工作流程如下：
 * 1. 检查删除的起始位置是否超出文件的实际大小。如果超出，打印错误信息并返回 -1。
 * 2. 根据删除的范围判断是否需要删除整个数据块。
 * 3. 初始化局部变量，获取文件的所有数据块信息。
 * 4. 根据 offset 计算要操作的数据块及块内偏移量。
 * 5. 使用回滚机制，如果需要删除整个数据块：
 *    - 如果是在块的开始位置，则逐块释放相应的数据块。
 *    - 如果不在块的开始位置，则清空当前块的数据，并继续回滚删除下一个块。
 * 6. 如果不需要删除整个数据块，则逐块删除内容，直到删除指定的字节数。
 * 7. 更新文件的 inode，并同步到磁盘。
 * 8. 释放分配的缓冲区并返回新的文件大小。
 */
int32_t file_remove_some_content(struct file* File,uint32_t offset,uint32_t size);

int32_t log_file_open(uint32_t inode_no, uint8_t flag);

int32_t log_get_free_slot_in_global(void);

#endif