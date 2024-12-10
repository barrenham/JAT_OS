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
#include "../include/fault.h"
#include "../include/ioqueue.h"
#include "../include/keyboard.h"
#include "../include/pipe.h"

extern uint8_t channel_cnt;
extern struct ide_channel channels[2];
extern struct list partition_list;
extern struct dir root_dir;
extern struct partition* cur_part;
extern struct file file_table[MAX_FILE_OPEN];


uint32_t fd_local2global(uint32_t local_fd){
    if(local_fd<0||local_fd>=MAX_FILES_OPEN_PER_PROC){
        return FAILED_FD;
    }
    struct task_struct* cur=running_thread();
    int32_t global_fd=cur->fd_table[local_fd];
    //ASSERT(global_fd>=0&&global_fd<MAX_FILE_OPEN);
    return (uint32_t)global_fd;
}

static char* path_parse(char* pathname,char* name_store){
    if(pathname[0]=='/'){
        while(*(++pathname)=='/');
    }
    while(*pathname!='/'&&*pathname!=0){
        *name_store++=*pathname++;
    }
    if(pathname[0]==0){
        *name_store++='\0';
        return NULL;
    }
    *name_store++='\0';
    return pathname;
}

static int search_file(const char* pathname,struct path_search_record* searched_record)
{
    if(!strcmp(pathname,"/")||!strcmp(pathname,"/.")||!strcmp(pathname,"/..")){
        searched_record->parent_dir=&root_dir;
        searched_record->file_type=FT_DIRECTORY;
        searched_record->searched_path[0]=0;
        return 0;
    }
    uint32_t path_len=strlen(pathname);
    ASSERT(pathname[0]=='/'&&path_len>1&&path_len<MAX_PATH_LEN);
    char* sub_path=(char*)pathname;
    struct dir* parent_dir=&root_dir;
    struct dir_entry dir_e;
    char name[MAX_FILE_NAME_LEN]={0};
    searched_record->parent_dir=parent_dir;
    searched_record->file_type=FT_UNKNOWN;
    uint32_t parent_inode_no=0;

    sub_path=path_parse(sub_path,name);
    while(name[0]){
        ASSERT(strlen(searched_record->searched_path)<512);
        strcat(searched_record->searched_path,"/");
        strcat(searched_record->searched_path,name);
        if(search_dir_entry(cur_part,parent_dir,name,&dir_e)){
            memset(name,0,MAX_FILE_NAME_LEN);
            if(sub_path!=NULL){
                sub_path=path_parse(sub_path,name);
            }
            if(dir_e.f_type==FT_DIRECTORY){
                parent_inode_no=parent_dir->inode->i_no;
                dir_close(parent_dir);
                parent_dir=dir_open(cur_part,dir_e.i_no);
                searched_record->parent_dir=parent_dir;
            }else if(dir_e.f_type==FT_REGULAR){
                searched_record->file_type=FT_REGULAR;
                return dir_e.i_no;
            }
        }else{
            return -1;
        }
    }
    dir_close(searched_record->parent_dir);

    searched_record->parent_dir=dir_open(cur_part,parent_inode_no);
    searched_record->file_type=FT_DIRECTORY;
    return dir_e.i_no;
}


int32_t path_depth_cnt(char* pathname){
    ASSERT(pathname!=NULL);
    char* p=pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth=0;
    p=path_parse(p,name);
    while(name[0]){
        depth++;
        memset(name,0,MAX_FILE_NAME_LEN);
        if(p){
            p=path_parse(p,name);
        }
    }
    return depth;
}

struct partition* cur_part;

static void partition_format(struct partition* part){
    struct disk* hd;
    uint32_t boot_sector_sects=1;
    uint32_t super_block_sects=1;
    uint32_t inode_bitmap_sects=DIV_ROUND_UP(MAX_FILES_PER_PART,BITS_PER_SECTOR);
    uint32_t inode_table_sects=DIV_ROUND_UP((MAX_FILES_PER_PART*sizeof(struct inode)),SECTOR_SIZE);
    uint32_t used_sects=boot_sector_sects+super_block_sects+inode_bitmap_sects+inode_table_sects;
    uint32_t free_sects=part->sec_cnt-used_sects;
    ASSERT(free_sects>=0);

    uint32_t block_bitmap_sects;
    block_bitmap_sects=DIV_ROUND_UP(free_sects,BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len=free_sects-block_bitmap_sects;
    ASSERT(block_bitmap_bit_len>=0);
    block_bitmap_sects=DIV_ROUND_UP(block_bitmap_bit_len,BITS_PER_SECTOR);
    ASSERT(block_bitmap_sects>=0);

    struct super_block sb;
    sb.magic=0x19590318;
    sb.sec_cnt=part->sec_cnt;
    sb.inode_cnt=MAX_FILES_PER_PART;
    sb.part_lba_base=part->start_lba;

    sb.block_bitmap_lba=sb.part_lba_base+2;
    sb.block_bitmap_sects=block_bitmap_sects;

    sb.inode_bitmap_lba=sb.block_bitmap_lba+sb.block_bitmap_sects;
    sb.inode_bitmap_sects=inode_bitmap_sects;

    sb.inode_table_lba=sb.inode_bitmap_lba+sb.inode_bitmap_sects;
    sb.inode_table_sects=inode_table_sects;

    sb.data_struct_lba=sb.inode_table_lba+inode_table_sects;
    sb.root_inode_no=0;
    sb.dir_entry_size=sizeof(struct dir_entry);

    printk(" %s info:\n",part->name);
    printk("    magic number: 0x%x\n",sb.magic);
    printk("    all_sectors: %d\n",part->sec_cnt);
    printk("    inode_cnt:   %d\n",sb.inode_cnt);
    printk("    inode_bitmap_sectors: %d\n",sb.inode_bitmap_sects);
    printk("    inode_table_sectors: %d\n",sb.inode_table_sects);
    printk("    block_bitmap_sectors: %d\n",sb.block_bitmap_sects);
    printk("    data start lba: %d\n",sb.data_struct_lba);

    hd=part->my_disk;

    ide_write(hd,part->start_lba+1,&sb,1);
    printk("    super_block input partition %s sector:%d\n",part->name,part->start_lba+1);

    uint32_t buf_size=(sb.block_bitmap_sects>=sb.inode_bitmap_sects)?\
                        sb.block_bitmap_sects:sb.inode_bitmap_sects;

    buf_size=((buf_size>=sb.inode_table_sects)?buf_size:sb.inode_table_sects)*SECTOR_SIZE;

    uint8_t* buf=(uint8_t*)sys_malloc(buf_size);

    buf[0]|=0x01;
    uint32_t block_bitmap_last_byte=block_bitmap_bit_len/8;
    uint8_t  block_bitmap_last_bit=block_bitmap_last_byte%8;
    uint32_t last_size=SECTOR_SIZE-(block_bitmap_last_byte%SECTOR_SIZE);
    memset(&buf[block_bitmap_last_byte],0xff,last_size);

    uint8_t bit_idx=0;
    while(bit_idx<=block_bitmap_last_bit){
        buf[block_bitmap_last_byte]&=~(1<<bit_idx++);
    }
    ide_write(hd,sb.block_bitmap_lba,buf,sb.block_bitmap_sects);

    memset(buf,0,buf_size);
    buf[0]|=0x01;
    ide_write(hd,sb.inode_bitmap_lba,buf,sb.inode_bitmap_sects);

    memset(buf,0,buf_size);
    struct inode* i=(struct inode*)buf;
    i->i_size=sb.dir_entry_size*2;
    i->i_no=0;
    i->i_sectors[0]=sb.data_struct_lba;
    ide_write(hd,sb.inode_table_lba,buf,sb.inode_table_sects);

    memset(buf,0,buf_size);
    struct dir_entry* p_de=(struct dir_entry*)buf;
    memcpy(p_de->filename,".",1);
    p_de->i_no=0;
    p_de->f_type=FT_DIRECTORY;
    p_de++;

    memcpy(p_de->filename,"..",2);
    p_de->i_no=0;
    p_de->f_type=FT_DIRECTORY;

    ide_write(hd,sb.data_struct_lba,buf,1);

    printk("    root_dir_lba:0x%x\n",sb.data_struct_lba);
    printk("    %s format done\n",part->name);
    sys_free(buf);
}

static bool mount_partition(struct list_elem* pelem,int arg){
    char* part_name=(char*)arg;
    struct partition* part=elem2entry(struct partition,part_tag,pelem);
    if(!strcmp(part->name,part_name)){
        cur_part=part;
        struct disk* hd=cur_part->my_disk;

        struct super_block* sb_buf=(struct super_block*)sys_malloc(SECTOR_SIZE);

        cur_part->sb=(struct super_block*)sys_malloc(sizeof(struct super_block));
        ASSERT(cur_part->sb!=NULL);
        memset(sb_buf,0,SECTOR_SIZE);
        ide_read(hd,cur_part->start_lba+1,sb_buf,1);
        memcpy(cur_part->sb,sb_buf,sizeof(struct super_block));
        cur_part->block_bitmap.bits=(uint8_t*)sys_malloc(sb_buf->block_bitmap_sects*SECTOR_SIZE);

        ASSERT(cur_part->block_bitmap.bits!=NULL);

        cur_part->block_bitmap.btmp_bytes_len=sb_buf->block_bitmap_sects*SECTOR_SIZE;
        ide_read(hd,sb_buf->block_bitmap_lba,cur_part->block_bitmap.bits,sb_buf->block_bitmap_sects);
        
        cur_part->inode_bitmap.bits=(uint8_t*)sys_malloc(sb_buf->inode_bitmap_sects*SECTOR_SIZE);
        ASSERT(cur_part->inode_bitmap.bits!=NULL);

        cur_part->inode_bitmap.btmp_bytes_len=sb_buf->inode_bitmap_sects*SECTOR_SIZE;
        ide_read(hd,sb_buf->inode_bitmap_lba,cur_part->inode_bitmap.bits,sb_buf->inode_bitmap_sects);

        list_init(&cur_part->open_inodes);

        printk("    inode table lba:0x%x\n",cur_part->sb->inode_table_lba);
        printk("    inode table sects:0x%x\n",cur_part->sb->inode_table_sects);
        printk("    root_dir start lba:0x%x\n",cur_part->sb->data_struct_lba);
        printk("mount %s done\n",part->name);
        return True;
    }
    return False;
}



void filesys_init(){
    uint8_t channel_no=0,dev_no,part_idx=0;

    struct super_block* sb_buf=(struct super_block*)sys_malloc(SECTOR_SIZE);

    ASSERT(sb_buf!=NULL);

    printk("searching filesystem ......\n");
    while(channel_no<channel_cnt){
        dev_no=0;
        while(dev_no<2){
            if(dev_no==0){
                dev_no++;
                continue;
            }
        struct disk* hd=&channels[channel_no].devices[dev_no];
        struct partition* part=hd->prim_parts;
        while(part_idx<12){
            if(part_idx==4){
                part=hd->logic_parts;
            }
            if(part->sec_cnt!=0){
                memset(sb_buf,0,SECTOR_SIZE);
                ide_read(hd,part->start_lba+1,sb_buf,1);
                if(sb_buf->magic==0x19590318){
                    printk("%s has filesystem\n",part->name);
                }else{
                    printk("formatting %s's partition %s......\n",hd->name,part->name);
                    partition_format(part);
                }
            }
            part_idx++;
            part++;
        }
        dev_no++;
        }
        channel_no++;
    }
    sys_free(sb_buf);
    char default_part[8]="sdb1";
    list_traversal(&partition_list,mount_partition,(int)default_part);
    open_root_dir(cur_part);

    uint32_t fd_idx=0;
    while(fd_idx<MAX_FILE_OPEN){
        file_table[fd_idx++].fd_inode=NULL;
    }
}

int32_t sys_open(const char* pathname,uint8_t flags){
    if(pathname[strlen(pathname)-1]=='/'){
        printk("cannot open a directory %s\n",pathname);
        return -1;
    }
    ASSERT(flags<=7);
    int32_t fd=-1;

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);

    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(searched_record.file_type==FT_DIRECTORY){
        printk("cannot open a directory with open(), use opendir() instead.\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }
    uint32_t path_searched_depth=path_depth_cnt(searched_record.searched_path);

    if(pathname_depth!=path_searched_depth){
        printk("search path failed, maybe subpath question\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    if(!found&&!(flags&O_CREAT)){
        printk("in path %s, file %s is not exist\n",
                searched_record.searched_path,
                (strrchr(searched_record.searched_path,'/')+1));
        dir_close(searched_record.parent_dir);
        return -1;
    }
    switch(flags&O_CREAT){
        case O_CREAT:
        {
            if(found){
                printk("already have file %s\n",(strrchr(searched_record.searched_path,'/')+1));
                dir_close(searched_record.parent_dir);
                return -1;
            }
            printk("creating file\n");
            fd=file_create(searched_record.parent_dir,(strrchr(searched_record.searched_path,'/')+1),flags);
            dir_close(searched_record.parent_dir);
            printk("created file %s\n",(strrchr(searched_record.searched_path,'/')+1));
            break;
        }
        default:
        {
            fd=file_open(inode_no,flags);
            dir_close(searched_record.parent_dir);
            break;
        }
    }
    return fd;
}

int32_t sys_close(int32_t fd){
    int32_t ret=-1;
    if(fd>2){
        if(is_pipe(fd)){
            if(--file_table[fd_local2global(fd)].fd_pos==0){
                mfree_page(PF_KERNEL,file_table[fd_local2global(fd)].fd_inode,1);
            }
            running_thread()->fd_table[fd]=-1;
            ret=0;
        }else{
            uint32_t _fd=fd_local2global(fd);
            ret=file_close(&file_table[_fd]);
            running_thread()->fd_table[fd]=-1;
        }
    }
    return ret;
}

int32_t sys_opendir(const char* pathname,uint8_t flags){
    ASSERT(flags<=7);
    int32_t fd=-1;

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);

    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(searched_record.file_type==FT_REGULAR){
        printk("cannot open a directory with opendir(), use open() instead.\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }
    uint32_t path_searched_depth=path_depth_cnt(searched_record.searched_path);

    if(pathname_depth!=path_searched_depth){
        printk("search path failed, maybe subpath question\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    if(!found&&!(flags&O_CREAT)){
        printk("in path %s, DIR %s is not exist\n",
                searched_record.searched_path,
                (strrchr(searched_record.searched_path,'/')+1));
        dir_close(searched_record.parent_dir);
        return -1;
    }
    switch(flags&O_CREAT){
        case O_CREAT:
        {
            if(found){
                printk("already have DIR %s\n",(strrchr(searched_record.searched_path,'/')+1));
                return -1;
            }
            printk("creating DIR\n");
            fd=dir_create(searched_record.parent_dir,(strrchr(searched_record.searched_path,'/')+1),flags);
            dir_close(searched_record.parent_dir);
            printk("created DIR %s\n",(strrchr(searched_record.searched_path,'/')+1));
        }
    }
    return fd;
}

int32_t user_file_write(int32_t fd,const void* buf,uint32_t bufsize){
    
    int32_t result=-1;
    if(fd==stdout_no){
        result=sys_write(fd,buf,0,bufsize);
        return result;
    }
    uint32_t _fd=fd_local2global(fd);
    if(!is_pipe(fd)){
        log_printk(FILE,"trying to write to file\n ");
        result=sys_write(fd,buf,file_table[_fd].fd_pos,bufsize);
        if(result!=-1){
            file_table[_fd].fd_pos+=bufsize;
        }
    }else{
        result=pipe_write(fd,buf,bufsize);
    }
    return result;
}

int32_t sys_write(int32_t fd,const void* buf,uint32_t offset,uint32_t bufsize){
    if(fd<0||fd==stdin_no){
        return -1;
    }
    if(fd==stdout_no){
        char tmp_buf[256]={0};
        ASSERT(bufsize<256);
        memcpy(tmp_buf,buf,bufsize);
        console_put_string(tmp_buf);
        return -1;
    }
    uint32_t _fd=fd_local2global(fd);
    struct file* wr_file=&file_table[_fd];
    if(wr_file->fd_flag&O_WRONLY||wr_file->fd_flag&O_RDWR){
        uint32_t bytes_written=file_write(wr_file,buf,offset,bufsize);
        return bytes_written;
    }else{
        console_put_string("sys_write: not allowed to write file without flag O_RDWR or O_WRONLY\n");
        return -1;
    }
}

int32_t sys_read(int32_t fd,const void* buf,uint32_t offset,uint32_t bufsize){
    if(fd<0){
        printk("sys_read: fd error\n");
        return -1;
    }
    ASSERT(buf!=NULL);
    if(fd==stdin_no){

    }
    uint32_t _fd=fd_local2global(fd);
    return file_read(&file_table[_fd],buf,offset,bufsize);
}

int32_t sys_seekp(int32_t fd,int32_t offset,enum whence wh_type){
    if(fd<=2){
        return -1;
    }
    uint32_t _fd=fd_local2global(fd);
    switch(wh_type){
        case SEEK_SET:
        {
            //ASSERT(offset<=file_table[_fd].fd_inode->i_size);
            if(offset>file_table[_fd].fd_inode->i_size){
                file_table[_fd].fd_pos=file_table[_fd].fd_inode->i_size;
            }else{
                file_table[_fd].fd_pos=offset;
            }
            
            break;
        }
        case SEEK_CUR:
        {
            ASSERT(offset+file_table[_fd].fd_pos<=file_table[_fd].fd_inode->i_size);
            file_table[_fd].fd_pos+=offset;
            break;
        }
        case SEEK_END:
        {
            ASSERT(offset<=0);
            ASSERT(-offset<=(int32_t)file_table[_fd].fd_inode->i_size);
            file_table[_fd].fd_pos=offset+(int32_t)(file_table[_fd].fd_inode->i_size);
            break;
        }
        default:
        {
            break;
        }
    }
    return file_table[_fd].fd_pos;
}

int32_t sys_remove_some_content(int32_t fd,int32_t offset,int32_t size){
    if(fd<3){
        console_put_string("sys_remove_some_content: fd error\n");
        return -1;
    }
    uint32_t _fd=fd_local2global(fd);
    if(_fd<0){
        return -1;
    }
    return file_remove_some_content(&file_table[_fd],offset,size);
}

int32_t sys_remove(const char* pathname){
    char name[MAX_FILE_NAME_LEN]={0};
    int idx=0;
    for(idx=strlen(pathname)-1;idx>=0;idx--){
        if(pathname[idx]=='/'){
            break;
        }
    }
    idx++;
    strcpy(name,pathname+idx);
    if(pathname[strlen(pathname)-1]=='/'){
        printk("temporary cannot remove a directory %s\n",pathname);
        return -1;
    }
    int32_t fd=-1;

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);

    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;

    if(found==False){
        printk("%s\n",pathname);
        printk("cannot find file!\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    if(searched_record.file_type==FT_DIRECTORY){
        printk("temporary cannot remove a directory %s\n",pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }
    uint32_t path_searched_depth=path_depth_cnt(searched_record.searched_path);

    if(pathname_depth!=path_searched_depth){
        printk("search path failed, maybe subpath question\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    struct inode* parent_inode=searched_record.parent_dir->inode;

    uint32_t secondary_lba=0;
    uint32_t* all_blocks=malloc(140*sizeof(uint32_t));
    memset(all_blocks,0,140*sizeof(uint32_t));
    uint8_t* io_buf=(uint8_t*)sys_malloc(BLOCK_SIZE);
    for(int i=0;i<13;i++){
        all_blocks[i]=parent_inode->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    uint32_t blocks_passed=0;
    while(all_blocks[blocks_passed]!=0){
        ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        struct dir_entry* dirs=(struct dir_entry*)io_buf;
        for(int i=0;i<(BLOCK_SIZE)/(sizeof(struct dir_entry));i++){
            if(!strcmp(dirs->filename,name)){
                memset(dirs,0,sizeof(struct dir_entry));
                ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
                break;
            }
            dirs++;
        }
        blocks_passed+=1;
    }
    dir_close(searched_record.parent_dir);
    struct file File;
    File.fd_inode=inode_open(cur_part,inode_no);
    file_remove_some_content(&File,0,File.fd_inode->i_size);
    inode_delete(cur_part,inode_no,io_buf);
    list_remove(&File.fd_inode->inode_tag);
    sys_free(all_blocks);
    sys_free(io_buf);
    return 0;
}


int32_t sys_dir_list(const char*pathname){

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);
    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(found==False){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    if(searched_record.file_type!=FT_DIRECTORY){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    struct inode* dir=inode_open(cur_part,inode_no);
    uint32_t secondary_lba;
    uint32_t* all_blocks=(uint32_t*)sys_malloc(140*sizeof(uint32_t));
    uint8_t* io_buf=(uint8_t*)sys_malloc(BLOCK_SIZE);
    for(int i=0;i<13;i++){
        all_blocks[i]=dir->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    for(int i=0;i<140&&all_blocks[i]!=0;i++){
        ide_read(cur_part->my_disk,all_blocks[i],io_buf,1);
        struct dir_entry* dirE=(struct dir_entry*)io_buf;
        int cnt=0;
        putchar('\n');
        while((cnt<(BLOCK_SIZE/(sizeof(struct dir_entry))))){
            if(dirE->filename[0]!='\0'){
                dirE->filename[MAX_FILE_NAME_LEN-1]='\0';
                printf("%s ",dirE->filename);
            }
            dirE++;
            cnt++;
        }
    }
    putchar('\n');
    inode_close(dir);
    dir_close(searched_record.parent_dir);
    sys_free(all_blocks);
    sys_free(io_buf);
    return 0;
}

int32_t sys_dir_list_info(const char*pathname){

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);
    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(found==False){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    if(searched_record.file_type!=FT_DIRECTORY){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    struct inode* dir=inode_open(cur_part,inode_no);
    uint32_t secondary_lba;
    uint32_t* all_blocks=(uint32_t*)sys_malloc(140*sizeof(uint32_t));
    memset(all_blocks,0,140*sizeof(uint32_t));
    uint8_t* io_buf=(uint8_t*)sys_malloc(BLOCK_SIZE);
    for(int i=0;i<13;i++){
        all_blocks[i]=dir->i_sectors[i];
        if(i==12&&all_blocks[i]!=0){
            secondary_lba=all_blocks[i];
            ide_read(cur_part->my_disk,all_blocks[i],all_blocks+12,1);
        }
    }
    for(int i=0;i<140&&all_blocks[i]!=0;i++){
        ide_read(cur_part->my_disk,all_blocks[i],io_buf,1);
        struct dir_entry* dirE=(struct dir_entry*)io_buf;
        int cnt=0;
        console_put_char('\n');
        while((cnt<(BLOCK_SIZE/(sizeof(struct dir_entry))))){
            if(dirE->filename[0]!='\0'){
                dirE->filename[MAX_FILE_NAME_LEN-1]='\0';
                char* filepath=sys_malloc(MAX_PATH_LEN);
                strcpy(filepath,pathname);
                if(filepath[strlen(filepath)-1]!='/'){
                    strcat(filepath,"/");
                }
                strcat(filepath,dirE->filename);
                file_descriptor fd=sys_open(filepath,O_RDONLY);
                printf("[%s size:%d B type: %s,inode:%d]\n",dirE->filename,sys_get_file_size(fd),dirE->f_type==FT_DIRECTORY?"DIRECTORY":(dirE->f_type==FT_REGULAR)?"REGULAR  ":"UNKNOWN  ",\
                dirE->i_no);
                sys_close(fd);
                sys_free(filepath);
            }
            dirE++;
            cnt++;
        }
    }
    putchar('\n');
    inode_close(dir);
    dir_close(searched_record.parent_dir);
    sys_free(all_blocks);
    sys_free(io_buf);
    return 0;
}


int32_t sys_delete_dir(const char* pathname){
    char name[MAX_FILE_NAME_LEN]={0};
    int idx=0;
    for(idx=strlen(pathname)-1;idx>=0;idx--){
        if(pathname[idx]=='/'){
            break;
        }
    }
    idx++;
    strcpy(name,pathname+idx);
    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);
    int inode_no=search_file(pathname,&searched_record);
    ASSERT(inode_no!=0);
    bool found=inode_no!=-1?True:False;
    if(!found){
        printk("dir not found\n");
        return -1;
    }
    if(searched_record.file_type!=FT_DIRECTORY){
        printk("not a DIR!\n");
        return -1;
    }
    struct inode* inode=inode_open(cur_part,inode_no);
    struct dir Dir;
    Dir.inode=inode;
    dir_delete(&Dir);
    inode_close(inode);
    uint32_t* all_blocks=(uint32_t*)sys_malloc(140);
    if(all_blocks==NULL){
        printk("dir_delete: sys_malloc for all_blocks failed\n");
        sys_free(all_blocks);
        return -1;
    }
    uint32_t  secondary_lba=0;
    for(int i=0;i<13;i++){
        all_blocks[i]=searched_record.parent_dir->inode->i_sectors[i];
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
        ide_read(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
        while(cnt<(BLOCK_SIZE/sizeof(struct dir_entry))&&dirE->f_type!=FT_UNKNOWN){
            if(!strcmp(dirE->filename,"..")||!strcmp(dirE->filename,".")){
                ;
            }else{
                if(!strcmp(dirE->filename,name)){
                    memset(dirE,0,sizeof(struct dir_entry));
                    ide_write(cur_part->my_disk,all_blocks[blocks_passed],io_buf,1);
                    goto delete_end;
                }
            }
            dirE++;
            cnt++;
        }
        blocks_passed+=1;
    }
delete_end:
    sys_free(io_buf);
    sys_free(all_blocks);
    dir_close(searched_record.parent_dir);
    return 0;
}



int32_t user_file_open(const char* pathname,uint8_t flags){
    log_printk(FILE,"trying to open file %s\n",pathname);
    return sys_open(pathname,flags);
}

int32_t user_mkdir(const char* pathname){
    log_printk(FILE,"trying to mkdir:%s\n",pathname);
    return sys_opendir(pathname,O_CREAT);
}

int32_t user_file_read(int32_t fd,const char* buf,uint32_t bufsize){
    if(fd==stdin_no){
        char* buffer=buf;
        uint32_t bytes_read=0;
        while(bytes_read<bufsize){
            *buffer=ioq_getchar(&kbd_buf);
            bytes_read++;
            buffer++;
        }
        return (bytes_read==0?-1:(int32_t)bytes_read);
    }
    if(fd<=0){
        return GENERAL_FAULT;
    }
    int32_t _fd=fd_local2global(fd);
    if(_fd==FAILED_FD){
        return CANNOT_FIND_FD_IN_PROCESS_STACK;
    }
    if(file_table[_fd].fd_inode==NULL){
        return CANNOT_FIND_INODE_IN_GLOBAL_FILE_TABLE;
    }
    int32_t read_cnt=-1;
    if(file_table[_fd].fd_flag!=PIPE_FLAG){
        read_cnt=sys_read(fd,buf,file_table[_fd].fd_pos,bufsize);
        if(read_cnt==GENERAL_FAULT){
            return CANNOT_READ_FILE;
        }
        file_table[_fd].fd_pos+=read_cnt;
    }else{
        read_cnt=pipe_read(fd,buf,bufsize);
    }
    return read_cnt;
}

int32_t user_file_seekp(int32_t fd,int32_t offset,enum whence wh_type){
    if(fd<=2){
        return GENERAL_FAULT;
    }
    int32_t _fd=fd_local2global(fd);
    if(_fd==FAILED_FD){
        return GENERAL_FAULT;
    }
    ASSERT(!is_pipe(fd));
    if(file_table[_fd].fd_inode==NULL){
        return CANNOT_FIND_INODE_IN_GLOBAL_FILE_TABLE;
    }
    log_printk(SYSCALL,"seek file to offset %d\n",offset);
    int32_t result=sys_seekp(fd,offset,wh_type);
    return result;
}

int32_t user_file_remove_some_content(int32_t fd,int32_t size){
    int32_t _fd=fd_local2global(fd);
    if(_fd==FAILED_FD){
        return CANNOT_FIND_FD_IN_PROCESS_STACK;
    }
    if(file_table[_fd].fd_inode==NULL){
        return CANNOT_FIND_INODE_IN_GLOBAL_FILE_TABLE;
    }
    int32_t _size=((file_table[_fd].fd_pos+size)>=file_table[_fd].fd_inode->i_size)?(file_table[_fd].fd_inode->i_size-file_table[_fd].fd_pos):(size);
    log_printk(SYSCALL,"file content removing\n");
    int32_t result=sys_remove_some_content(fd,file_table[_fd].fd_pos,_size);
    if(result==GENERAL_FAULT){
        return CANNOT_REMOVE_SOME_COTENT_FROM_FILE;
    }
    file_table[_fd].fd_pos+=size;
    return size;
}

int32_t user_delete(const char* pathname){
    log_printk(SYSCALL,"trying to delete %s\n",pathname);
    int32_t result=sys_remove(pathname);
    if(result==-1){
        result=sys_delete_dir(pathname);
    }
    return result;
}

int32_t user_file_close(int32_t fd){
    if(fd<=2){
        return CANNOT_FIND_FD_IN_PROCESS_STACK;
    }
    int32_t _fd=fd_local2global(fd);
    if(_fd==FAILED_FD){
        return CANNOT_FIND_FD_IN_PROCESS_STACK;
    }
    if(file_table[_fd].fd_inode==NULL){
        return CANNOT_FIND_INODE_IN_GLOBAL_FILE_TABLE;
    }
    log_printk(SYSCALL,"close file descriptor %d\n",fd);
    if(file_table[_fd].fd_inode->i_open_cnts==1){
        log_printk(DEVICE,"sync file to disk %s\n",cur_part->my_disk->name);
    }
    int32_t result=sys_close(fd);
    return result;
}

filesize sys_get_file_size(file_descriptor fd){
    if(fd<0){
        return GENERAL_FAULT;
    }
    file_descriptor _fd=fd_local2global(fd);
    if(_fd==FAILED_FD){
        return GENERAL_FAULT;
    }
    ASSERT(file_table[_fd].fd_inode!=NULL);
    if(file_table[_fd].fd_inode->i_size >(uint32_t)70*1024){
        return GENERAL_FAULT;
    }
    log_printk(SYSCALL,"getting file size\n");
    return file_table[_fd].fd_inode->i_size;
}



enum file_types sys_get_file_attribute(const char* pathname){
    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);
    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(found==False){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    dir_close(searched_record.parent_dir);
    log_printk(SYSCALL,"getting file type\n");
    return searched_record.file_type;
}



int32_t sys_open_log(const char* pathname,uint8_t flags){
    if(pathname[strlen(pathname)-1]=='/'){
        return -1;
    }
    ASSERT(flags<=7);
    int32_t fd=-1;

    struct path_search_record searched_record;
    memset(&searched_record,0,sizeof(struct path_search_record));

    uint32_t pathname_depth=path_depth_cnt((char*)pathname);

    int inode_no=search_file(pathname,&searched_record);
    bool found=inode_no!=-1?True:False;
    if(searched_record.file_type==FT_DIRECTORY){
        dir_close(searched_record.parent_dir);
        return -1;
    }
    uint32_t path_searched_depth=path_depth_cnt(searched_record.searched_path);

    if(pathname_depth!=path_searched_depth){
        dir_close(searched_record.parent_dir);
        return -1;
    }

    if(!found&&!(flags&O_CREAT)){

        dir_close(searched_record.parent_dir);
        return -1;
    }
    {
        fd=log_file_open(inode_no,flags);
        dir_close(searched_record.parent_dir);
    }
    return fd;
}

