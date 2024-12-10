#include "../include/global.h"
#include "../include/thread.h"
#include "../include/io.h"
#include "../include/print.h"
#include "../include/interrupt.h"
#include "../include/memory.h"
#include "../include/userprog.h"
#include "../include/process.h"
#include "../include/string.h"
#include "../include/console.h"
#include "../include/debug.h"
#include "../include/list.h"
#include "../include/stdio.h"
#include "../include/file.h"
#include "../include/inode.h"
#include "../include/pipe.h"
#include "../include/syscall.h"
#include "../include/stdio-kernel.h"

#define TASK_NAME_LEN 16

extern void intr_exit(void);
typedef uint32_t ELF32_Word,ELF32_Addr,ELF32_Off;
typedef uint16_t ELF32_Half;

struct ELF32_Ehdr{
    uint8_t         e_ident[16];
    ELF32_Half      e_type;
    ELF32_Half      e_machine;
    ELF32_Word      e_version;
    ELF32_Addr      e_entry;
    ELF32_Off       e_phoff;
    ELF32_Off       e_shoff;
    ELF32_Word      e_flags;
    ELF32_Half      e_ehsize;
    ELF32_Half      e_phentsize;
    ELF32_Half      e_phnum;
    ELF32_Half      e_shentsize;
    ELF32_Half      e_shnum;
    ELF32_Half      e_shstrndx;
} __attribute__((packed));

struct ELF32_Phdr{
    ELF32_Word      p_type;
    ELF32_Off       p_offset;
    ELF32_Addr      p_vaddr;
    ELF32_Addr      p_paddr;
    ELF32_Word      p_filesize;
    ELF32_Word      p_flags;
    ELF32_Word      p_align;
} __attribute__((packed));

enum segment_type{
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
};

static bool segment_load(int32_t fd,uint32_t offset,uint32_t filesize,uint32_t vaddr){
    uint32_t vaddr_first_page=vaddr&0xfffff000;
    uint32_t size_in_first_page=PG_SIZE-(vaddr&0x00000fff);
    uint32_t occupy_pages=0;
    if(filesize>size_in_first_page){
        uint32_t left_size=filesize-size_in_first_page;
        occupy_pages=DIV_ROUND_UP(left_size,PG_SIZE)+1;
    }else{
        occupy_pages=1;
    }
    uint32_t page_idx=0;
    uint32_t vaddr_page=vaddr_first_page;
    while(page_idx<occupy_pages){
        uint32_t* pde=pde_ptr(vaddr_page);
        uint32_t* pte=pte_ptr(vaddr_page);
        if(!(*pde&0x00000001)||!(*pte&0x00000001)){
            if(get_a_page(PF_USER,vaddr_page)==NULL){
                return False;
            }
        }
        vaddr_page+=PG_SIZE;
        page_idx++;
    }
    seekp(fd,offset,SEEK_SET);
    read(fd,(void*)vaddr,filesize);
    return True;
}

static int32_t load(const char* pathname){
    int32_t ret=-1;
    struct ELF32_Ehdr elf_header;
    struct ELF32_Phdr prog_header;
    memset(&elf_header,0,sizeof(struct ELF32_Ehdr));

    int32_t fd=openFile(pathname,O_RDONLY);
    if(fd==-1){
        printk("false 1 in load\n");
        return -1;
    }
    seekp(fd,0,SEEK_SET);
    if(read(fd,&elf_header,sizeof(struct ELF32_Ehdr))!=sizeof(struct ELF32_Ehdr)){
        printk("false 2 in load\n");
        ret=-1;
        goto done;
    }
    if(memcmp(elf_header.e_ident,"\177ELF\1\1\1",7)||\
        elf_header.e_type!=2||\
        elf_header.e_machine!=3)
    {
        printk("false 3 in load\n");
        ret=-1;
        goto done;
    }
    ELF32_Off prog_header_offset=elf_header.e_phoff;
    ELF32_Half prog_header_size=elf_header.e_phentsize;
    uint32_t prog_idx=0;
    while(prog_idx<elf_header.e_phnum){
        seekp(fd,prog_header_offset,SEEK_SET);
        if(read(fd,&prog_header,prog_header_size)!=prog_header_size){
            printk("false 4 in load\n");
            ret=-1;
            goto done;
        }
        if(PT_LOAD==prog_header.p_type){
            if(!segment_load(fd,prog_header.p_offset,prog_header.p_filesize,prog_header.p_vaddr)){
                printk("false 5 in load\n");
                ret=-1;
                goto done;
            }
        }
        prog_header_offset+=elf_header.e_phentsize;
        prog_idx++;
    }
    ret=elf_header.e_entry;
done:
    closeFile(fd);
    return ret;
}

int32_t sys_execv(const char* path,const char* argv[]){
    uint32_t argc=0;
    while(argv[argc]){
        argc++;
    }
    int32_t entry_point=load(path);
    if(entry_point==-1){
        return -1;
    }
    int pathptr=strlen(path);
    while(pathptr>0){
        if(path[pathptr]=='/'){
            pathptr++;
            break;
        }
        pathptr--;
    }
    struct task_struct* cur=running_thread();
    memcpy(cur->name,&path[pathptr],TASK_NAME_LEN);
    cur->name[TASK_NAME_LEN-1]='\0';

    struct intr_stack* intr_0_stack=(struct intr_stack*)\
    ((uint32_t)cur+PG_SIZE-sizeof(struct intr_stack));
    intr_0_stack->ebx=(int32_t)argv;
    intr_0_stack->ecx=argc;
    intr_0_stack->eip=(void*)entry_point;
    intr_0_stack->esp=(void*)0xc0000000;

    asm volatile("movl %0, %%esp; jmp intr_exit" : :"g"(intr_0_stack):"memory");
    return 0;
}