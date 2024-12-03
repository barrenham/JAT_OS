#ifndef HISTORY_H
#define HISTORY_H

#include "list.h"
#include "stdint.h"
#include "shell.h"

#define MAX_HIS_CMD_CNT  10

struct history_elem{
    struct list_elem history_tag;
    char   cmd_line[cmd_len];
};

struct history{
    struct list history_list;
    int32_t cnt;
    struct history_elem* now_history_elem;
};



int32_t history_init(struct history* phistory);
int32_t history_push(struct history* phistory,char* cmd_line);
char* history_get_next(struct history* phistory);
char* history_get_prev(struct history* phistory);

#endif