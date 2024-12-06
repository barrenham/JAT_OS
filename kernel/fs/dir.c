#include "../include/file.h"
#include "../include/inode.h"
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

extern struct partition* cur_part;
struct dir root_dir;
extern struct file file_table[MAX_FILE_OPEN];

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
    uint32_t block_cnt=140;
    uint32_t* all_blocks=(uint32_t*)sys_malloc(48+512);
    if(all_blocks==NULL){
        printk("search_dir_entry: sys_malloc for all_blocks failed");
        sys_free(all_blocks);
        return False;
    }
    uint32_t block_idx=0;
    while(block_idx<12){
        all_blocks[block_idx]=pdir->inode->i_sectors[block_idx];
        block_idx++;
    }
    block_idx=0;

    if(pdir->inode->i_sectors[12]!=0){
        ide_read(part->my_disk,pdir->inode->i_sectors[12],all_blocks+12,1);
    }

    uint8_t* buf=(uint8_t*)sys_malloc(SECTOR_SIZE);
    struct dir_entry* p_de=(struct dir_entry*)buf;
    uint32_t dir_entry_size=part->sb->dir_entry_size;
    uint32_t dir_entry_cnt=SECTOR_SIZE/dir_entry_size;

    while(block_idx<block_cnt){
        if(all_blocks[block_idx]==0){
            block_idx++;
            continue;
        }
        ide_read(part->my_disk,all_blocks[block_idx],buf,1);
        uint32_t dir_entry_idx=0;
        while(dir_entry_idx<dir_entry_cnt){
            if(!strcmp(p_de->filename,name)){
                memcpy(dir_e,p_de,dir_entry_size);
                sys_free(buf);
                sys_free(all_blocks);
                return True;
            }
            dir_entry_idx++;
            p_de++;
        }
        block_idx++;
        p_de=(struct dir_entry*)buf;
        memset(buf,0,SECTOR_SIZE);
    }
    sys_free(buf);
    sys_free(all_blocks);
    return False;
}

void dir_close(struct dir* dir){
    if(dir==&root_dir||((uint32_t)dir==(0xC0000000|((uint32_t)(&root_dir))))){
        return;
    }
    if(dir==NULL){
        return;
    }
    inode_close(dir->inode);
    sys_free(dir);
}

void create_dir_entry(char* filename,uint32_t inode_no,uint8_t file_type,struct dir_entry* p_de){
    ASSERT(strlen(filename)<=MAX_FILE_NAME_LEN);

    memcpy(p_de->filename,filename,strlen(filename));
    p_de->i_no=inode_no;
    p_de->f_type=file_type;
}

bool sync_dir_entry(struct dir* parent_dir,
                    struct dir_entry* p_de,
                    void* io_buf)
{
    struct inode* dir_inode=parent_dir->inode;
    uint32_t dir_size=dir_inode->i_size;
    uint32_t dir_entry_size=cur_part->sb->dir_entry_size;

    ASSERT(dir_size%dir_entry_size==0);

    uint32_t dir_entry_per_sec=(512/dir_entry_size);

    uint32_t block_lba=-1;

    uint8_t block_idx=0;
    uint32_t all_blocks[140]={0};

    while(block_idx<12){
        all_blocks[block_idx]=dir_inode->i_sectors[block_idx];
        block_idx++;
    }
    if(dir_inode->i_sectors[12]!=0){
        uint32_t* buf=sys_malloc(SECTOR_SIZE);
        ide_read(cur_part->my_disk,dir_inode->i_sectors[12],buf,1);
        for(int i=0;i<128;i++){
            all_blocks[block_idx+i]=*(((uint32_t*)buf)+i);
        }
    }

    struct dir_entry* dir_e=(struct dir_entry*)io_buf;
    int32_t block_bitmap_idx=-1;

    block_idx=0;
    while(block_idx<13){
        block_bitmap_idx=-1;
        if(all_blocks[block_idx]==0){
            block_lba=block_bitmap_alloc(cur_part);
            if(block_lba==-1){
                printk("alloc block bitmap for sync_dir_entry failed\n");
                return False;
            }
            block_bitmap_idx=block_lba-cur_part->sb->data_struct_lba;
            ASSERT(block_bitmap_idx!=-1);
            bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);

            block_bitmap_idx=-1;
            if(block_idx<12){
                dir_inode->i_sectors[block_idx]=all_blocks[block_idx]=block_lba;
            }else if(block_idx==12){
                if(dir_inode->i_sectors[12]==0){
                    dir_inode->i_sectors[12]=block_lba;
                    block_lba=-1;
                    block_lba=block_bitmap_alloc(cur_part);
                    if(block_lba==-1){
                        block_bitmap_idx=dir_inode->i_sectors[12]-cur_part->sb->data_struct_lba;
                        bitmap_set(&cur_part->block_bitmap,block_bitmap_idx,0);
                        dir_inode->i_sectors[12]=0;
                        printk("alloc block bitmap for sync_dir_entry failed\n");
                        return False;
                    }
                    block_bitmap_idx=block_lba-cur_part->sb->data_struct_lba;
                    ASSERT(block_bitmap_idx!=-1);
                    bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
                    all_blocks[12]=block_lba;
                    ide_write(cur_part->my_disk,dir_inode->i_sectors[12],all_blocks+12,1);
                }else{
                    all_blocks[block_idx]=block_lba;
                }
            }
            memset(io_buf,0,512);
            memcpy(io_buf,p_de,dir_entry_size);
            ide_write(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
            return True;
        }
        ide_read(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
        uint8_t dir_entry_idx=0;
        while(dir_entry_idx<dir_entry_per_sec){
            if((dir_e+dir_entry_idx)->f_type==FT_UNKNOWN){
                memcpy(dir_e+dir_entry_idx,p_de,dir_entry_size);
                ide_write(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
                dir_inode->i_size+=dir_entry_size;
                return True;
            }
            dir_entry_idx++;
        }
        block_idx++;
    }
    while(block_idx<140){
        ide_read(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
        uint8_t dir_entry_idx=0;
        while(dir_entry_idx<dir_entry_per_sec){
            if((dir_e+dir_entry_idx)->f_type==FT_UNKNOWN){
                memcpy(dir_e+dir_entry_idx,p_de,dir_entry_size);
                ide_write(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
                dir_inode->i_size+=dir_entry_size;
                return True;
            }
            dir_entry_idx++;
        }
        block_idx++;
    }
    printk("directory is full!\n");
    return False;
}


int32_t dir_create(struct dir* parent_dir,char* filename,uint8_t flag)
{
    void* io_buf=sys_malloc(1024);
    if(io_buf==NULL){
        printk("in file_creat: sys_malloc for io_buf failed\n");
        sys_free(io_buf);
        return -1;
    }
    uint8_t rollback_step=0;

    int32_t inode_no=inode_bitmap_alloc(cur_part);
    if(inode_no==-1){
        printk("in file_create: allocate inode failed\n");
        sys_free(io_buf);
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

    create_dir_entry(filename,inode_no,FT_DIRECTORY,&new_dir_entry);
    if(!sync_dir_entry(parent_dir,&new_dir_entry,io_buf)){
        printk("sync dir_entry to disk failed\n");
        rollback_step=3;
        goto rollback;
    }
    memset(io_buf,0,1024);
    inode_sync(cur_part,parent_dir->inode,io_buf);

    new_file_inode->i_sectors[0]=block_bitmap_alloc(cur_part);
    ide_read(cur_part->my_disk,new_file_inode->i_sectors[0],io_buf,1);
    memset(io_buf,0,1024);
    struct dir_entry* dirE=(struct dir_entry*)io_buf;
    dirE->f_type=FT_DIRECTORY;
    dirE->i_no=new_file_inode->i_no;
    strcpy(dirE->filename,".");
    dirE++;
    dirE->f_type=FT_DIRECTORY;
    dirE->i_no=parent_dir->inode->i_no;
    strcpy(dirE->filename,"..");
    ide_write(cur_part->my_disk,new_file_inode->i_sectors[0],io_buf,1);
    bitmap_sync(cur_part,new_file_inode->i_sectors[0]-cur_part->sb->data_struct_lba,BLOCK_BITMAP);
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


int32_t dir_delete(struct dir* cur_dir)
{
    ASSERT(cur_dir!=NULL&&cur_dir->inode!=NULL&&cur_dir->inode->i_no!=0);
    uint32_t* all_blocks=(uint32_t*)sys_malloc(140);
    if(all_blocks==NULL){
        printk("dir_delete: sys_malloc for all_blocks failed\n");
        sys_free(all_blocks);
        return -1;
    }
    uint32_t  secondary_lba=0;
    for(int i=0;i<13;i++){
        all_blocks[i]=cur_dir->inode->i_sectors[i];
        if(all_blocks[i]!=0&&i==12){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    uint32_t blocks_passed=0;
    uint8_t* io_buf=(uint8_t*)sys_malloc(BLOCK_SIZE);
    while(all_blocks[blocks_passed]!=0){
        struct dir_entry* dirE=(struct dir_entry*)io_buf;
        int cnt=0;
        while(cnt<(BLOCK_SIZE/sizeof(struct dir_entry))&&dirE->f_type!=FT_UNKNOWN){
            if(!strcmp(dirE->filename,"..")||!strcmp(dirE->filename,".")){
                ;
            }else{
                switch(dirE->f_type){
                    case FT_DIRECTORY:
                    {
                        struct dir Dir;
                        Dir.inode=inode_open(cur_part,dirE->i_no);
                        dir_delete(&Dir);
                        printf("[removed %s]",dirE->filename);
                        inode_close(Dir.inode);
                        break;
                    }
                    case FT_REGULAR:
                    {   
                        struct inode* inode=inode_open(cur_part,dirE->i_no);
                        struct file File;
                        File.fd_inode=inode;    
                        file_remove_some_content(&File,0,inode->i_size);
                        inode_bitmap_dealloc(cur_part,inode->i_no);
                        bitmap_sync(cur_part,inode->i_no,INODE_BITMAP);
                        printf("[removed %s]",dirE->filename);
                        inode_close(inode);
                        break;
                    }
                }
            }
            dirE++;
            cnt++;
        }
        blocks_passed+=1;
    }
    inode_bitmap_dealloc(cur_part,cur_dir->inode->i_no);
    bitmap_sync(cur_part,cur_dir->inode->i_no,INODE_BITMAP);
    sys_free(all_blocks);
    sys_free(io_buf);
}
