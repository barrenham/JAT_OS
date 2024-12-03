#include "../include/stdint.h"
#include "../include/debug.h"
#include "../include/file.h"
#include "../include/stdio.h"
#include "../include/fs.h"
#include "../include/syscall.h"
#include "../include/thread.h"
#include "../include/shell.h"
#include "../include/io.h"
#include "../include/math.h"
#include "../include/interrupt.h"
#include "../include/inode.h"
#include "../include/userprog.h"
#include "../include/exec.h"
#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/list.h"
#include "../include/history.h"

int32_t history_init(struct history* phistory){
    list_init(&(phistory->history_list));
    phistory->cnt=0;
    phistory->now_history_elem=NULL;
    return 0;
}

static history_elem_init(struct history_elem* phe,char* cmd_line){
    phe->history_tag.next=phe->history_tag.prev=NULL;
    strcpy(phe->cmd_line,cmd_line);
}

int32_t history_push(struct history* phistory,char* cmd_line){
    if((phistory->cnt) >= MAX_HIS_CMD_CNT){
        struct history_elem* throwed=phistory->history_list.tail.prev;
        list_remove(&(throwed->history_tag));
        (phistory->cnt)--;
        free(throwed);
    }
    struct history_elem* new_elem=malloc(sizeof(struct history_elem));
    history_elem_init(new_elem,cmd_line);
    list_push(&(phistory->history_list),&(new_elem->history_tag));
    phistory->now_history_elem=new_elem;
    (phistory->cnt)++;
    return 0;
}
char* history_get_next(struct history* phistory){
    if(phistory->now_history_elem->history_tag.prev==NULL||phistory->now_history_elem->history_tag.prev==(void*)0xC0000000){
        return NULL;
    }
    phistory->now_history_elem=phistory->now_history_elem->history_tag.prev;
    return phistory->now_history_elem->cmd_line;
}
char* history_get_prev(struct history* phistory){
    if(phistory->now_history_elem->history_tag.next==NULL||phistory->now_history_elem->history_tag.next==(void*)0xC0000000){
        return NULL;
    }
    phistory->now_history_elem=phistory->now_history_elem->history_tag.next;
    return phistory->now_history_elem->cmd_line;
}