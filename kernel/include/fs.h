#ifndef __FS_H
#define __FS_H

#include "inode.h"
#include "file.h"
#include "ide.h"

#define MAX_FILES_PER_PART  4096
#define BITS_PER_SECTOR     2096
#define SECTOR_SIZE         512
#define BLOCK_SIZE          SECTOR_SIZE

#define MAX_PATH_LEN        512

#define FAILED_FD           -1

typedef int32_t  filesize;
typedef int32_t  file_descriptor;

enum file_types{
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

enum oflags{
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT=4
};

enum whence{
    SEEK_SET=1,
    SEEK_CUR,
    SEEK_END
};

struct path_search_record{
    char searched_path[MAX_PATH_LEN];
    struct dir* parent_dir;
    enum file_types file_type;
};

void filesys_init();

int32_t path_depth_cnt(char* pathname);


int32_t sys_open(const char* pathname,uint8_t flags);

int32_t sys_opendir(const char* pathname,uint8_t flags);

int32_t sys_close(int32_t fd);

int32_t user_file_write(int32_t fd,const void* buf,uint32_t bufsize);

int32_t sys_write(int32_t fd,const void* buf,uint32_t offset,uint32_t bufsize);

int32_t sys_read(int32_t fd,const void* buf,uint32_t offset,uint32_t bufsize);

int32_t sys_seekp(int32_t fd,int32_t offset,enum whence wh_type);

int32_t sys_remove_some_content(int32_t fd,int32_t offset,int32_t size);

int32_t sys_remove(const char* pathname);

int32_t sys_opendir(const char* pathname,uint8_t flags);

int32_t sys_dir_list(const char*pathname);

int32_t sys_delete_dir(const char* pathname);


int32_t user_file_open(const char* pathname,uint8_t flags);

int32_t user_mkdir(const char* pathname);

int32_t user_file_read(int32_t fd,const char* buf,uint32_t bufsize);

int32_t user_file_seekp(int32_t fd,int32_t offset,enum whence wh_type);

int32_t user_file_remove_some_content(int32_t fd,int32_t size);

int32_t user_delete(const char* pathname);

int32_t user_file_close(int32_t fd);

uint32_t fd_local2global(uint32_t local_fd);

filesize sys_get_file_size(file_descriptor fd);

enum file_types sys_get_file_attribute(const char* pathname);

int32_t sys_dir_list_info(const char*pathname);

int32_t sys_open_log(const char* pathname,uint8_t flags);

void sys_set_file_attr(file_descriptor fd, int flags);

int sys_get_file_attr(file_descriptor fd);

#endif