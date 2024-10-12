#include "../include/file.h"
#include "../include/dir.h"
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

struct dir root_dir;

void open_root_dir(struct partition* part){
    root_dir.inode=inode_open(part,part->sb->root_inode_no);
    root_dir.dir_pos=0;
}

struct dir* dir_open(struct partition* part,uint32_t inode_no){
    struct dir* pdir=(struct dir*)sys_malloc(sizeof(struct dir));
    pdir->inode=inode_open(part,inode_no);
    pdir->dir_pos=0;
    return pdir;
}


bool search_dir_entry(struct partition* part,struct dir* pdir,const char* name,struct dir_entry* dir_e){
    
}