#ifndef FILE_H
#define FILE_H
#include"stdint.h"

struct file{
    uint32_t fd_pos;
    struct inode* fd_inode;
};

enum std_fd{
    stdin_no,
    stdout_no,
    stderr_no
};

enum bitmap_type{
    INODE_BITMAP,
    BLOCK_BITMAP,
};

#define MAX_FILE_OPEN 32

#endif