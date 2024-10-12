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

extern uint8_t channel_cnt;
extern struct ide_channel channels[2];
extern struct list partition_list;

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
    printk("    data_struct start lba: %d\n",sb.data_struct_lba);

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
}

