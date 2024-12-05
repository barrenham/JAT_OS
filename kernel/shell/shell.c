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
#include "../include/history.h"

#include "../include/editor.h"
#include "../include/print.h"

#define cmd_len 128
#define MAX_ARG_NR 16
#define MAX_PATH_LENGTH cmd_len
#define MAX_CMD_LENGTH cmd_len

extern struct file file_table[MAX_FILE_OPEN];
extern struct partition *cur_part;

static int atoi_10(const char *str)
{
    int result = 0;
    int sign = 1; // 默认是正数
    int i = 0;

    // 跳过前面的空白字符
    while (str[i] == ' ')
    {
        i++;
    }

    // 处理符号
    if (str[i] == '-')
    {
        sign = -1;
        i++;
    }
    else if (str[i] == '+')
    {
        i++; // 如果是正号，直接跳过
    }

    // 逐个字符转换为数字
    while (str[i] >= '0' && str[i] <= '9')
    {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    // 返回结果，考虑符号
    return result * sign;
}

extern struct list thread_all_list;
static char cmd_line[cmd_len] = {0};
static char cmd_line_kill_bat[cmd_len] = {0};
static char cmd_line_ps_bat[cmd_len] = {0};
static char cmd_line_cat_bat[cmd_len] = {0};
static char cmd_line_exec_bat[cmd_len] = {0};
static char cmd_line_rm_bat[cmd_len] = {0};
static char cmd_line_touch_bat[cmd_len] = {0};
static char cmd_line_edit_bat[cmd_len] = {0};

static struct history cmd_history;

static bool isChar(c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool isDigit(c)
{
    return (c >= '0' && c <= '9');
}

char cwd_cache[64] = {0};

void print_prompt(void)
{
    printf("[user&localhost %s]$ ", cwd_cache);
}

static void process_cat_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 3;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
    {
        i++;
    }
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    strcpy(buf, cwd_cache);
    idx = strlen(buf);
    if (buf[idx - 1] != '/')
    {
        buf[idx++] = '/';
    }
    while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
    {
        buf[idx++] = cmd_line[i++];
    }

    buf[idx] = '\0'; // 终止字符串
    if (get_file_type(buf) != FT_REGULAR)
    {
        return;
    }
    file_descriptor fd = openFile(buf, O_RDONLY);
    seekp(fd, 0, SEEK_SET);
    char read_buf[201] = {0};
    int read_cnt = 0;
    while ((read_cnt = read(fd, read_buf, 200)) >= 0)
    {
        for (int i = 0; i < read_cnt; i++)
        {
            printf("%c", read_buf[i]);
        }
    }
    closeFile(fd);
}

static void process_mkdir_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 5; // 假设命令行从 'mkdir' 后开始

    // 跳过多余的空格
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
    {
        i++;
    }

    // 解析路径，直到遇到无效字符或者超出最大长度
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;

    // 将当前工作目录复制到缓冲区
    strcpy(buf, cwd_cache);
    idx = strlen(buf);
    if (buf[idx - 1] != '/')
    {
        buf[idx++] = '/';
    }

    // 处理路径中的每个字符
    while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
    {
        buf[idx++] = cmd_line[i++];
    }

    buf[idx] = '\0'; // 终止字符串

    // 尝试创建目录
    if (mkdir(buf) > 0)
    {
        printf("Directory created: %s\n", buf);
    }
    else
    {
        printf("Failed to create directory: %s\n", buf);
    }
}

static void process_kill_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 4;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
    {
        i++;
    }
    pid_t pid=atoi_10(&cmd_line[i]);
    struct list_elem* elem=thread_all_list.head.next;
    while(elem->next!=NULL){
        struct task_struct* pcb=elem2entry(struct task_struct,all_list_tag,elem);
        if(pcb==running_thread()||(!strcmp(pcb->name,"thread_cleaner"))||(!strcmp(pcb->name,"shell"))){
            elem=elem->next;
            continue;
        }
        if (pcb->pid == pid)
        {
            enum intr_status old_status = intr_disable();
            uint32_t *pointer = (pcb->self_kstack);
            if (pcb->pgdir == NULL)
            {
                pointer[4] = thread_exit;
            }
            else
            {
                pointer[4] = process_exit;
            }
            intr_set_status(old_status);
        }
        elem = elem->next;
    }
}

static void process_cd_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 2; // 假设命令行从 'cd' 后开始
    int i_bat = 0;

    // 跳过多余的空格
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
    {
        i++;
    }

    i_bat = i;

    // 解析路径，直到遇到无效字符或者超出最大长度
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    bool enable = True;

    // 如果路径不为空，进行路径处理
    if (cmd_line[i] != '\0')
    {
        switch (cmd_line[i])
        {
        case '/': // 绝对路径
            buf[idx++] = cmd_line[i];
            i++;
            break;

        default: // 相对路径
            // 当前路径为 cwd_cache
            strcpy(buf, cwd_cache);
            idx = strlen(buf);
            if (buf[idx - 1] != '/')
            {
                buf[idx++] = '/';
            }
            break;
        }

        // 处理路径中的每个字符
        while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        {
            if (cmd_line[i] == '/')
            {
                // 遇到路径分隔符
                if (idx > 1 && buf[idx - 1] != '/')
                {
                    buf[idx++] = '/';
                }
                i++;
            }
            else if (cmd_line[i] == '.')
            {
                if (cmd_line[i + 1] == '.')
                {
                    // 遇到"../"：返回上级目录
                    if (idx > 1)
                    {
                        idx -= 2;
                        while (idx > 0 && buf[idx] != '/')
                        {
                            idx--;
                        }
                        if (idx > 0)
                        {
                            idx++;
                        }
                    }
                    if (idx == 0)
                    { // Special case for reaching root "/"
                        buf[idx++] = '/';
                    }
                    i += 2;
                }
                else
                {
                    // 当前目录符号"."忽略
                    if (cmd_line[i - 1] != '/')
                    {
                        buf[idx++] = cmd_line[i];
                    }
                    i++;
                }
            }
            else if (isChar(cmd_line[i]) || isDigit(cmd_line[i]))
            {
                // 处理有效的字符（字母数字）
                buf[idx++] = cmd_line[i];
                i++;
            }
            else
            {
                // 无效字符，忽略
                enable = False;
                break;
            }
        }

        buf[idx] = '\0'; // 终止字符串

        // 检查路径是否有效
        if (get_file_type(buf) != FT_DIRECTORY)
        {
            printf("Invalid path: %s\n", buf);
            enable = False;
        }

        // 如果路径有效，更新当前目录缓存
        if (enable)
        {
            strcpy(cwd_cache, buf);
            printf("Changed directory to: %s\n", cwd_cache);
        }
    }
    else
    {
        printf("No directory specified.\n");
    }
}


static void readline(char* buf,int32_t count){
    ASSERT(buf!=NULL&&count>0);
    char* pos=buf;
    while(read(stdin_no,pos,1)!=-1&&(pos-buf)<count){
        switch(*pos){
            case '\n':
            case '\r':
            {
                *pos=0;
                putchar('\n');
                return;
            }
            case '\b':
            {
                if(buf[0]!='\b'){
                    --pos;
                    putchar('\b');
                }
                break;
            }
            case ((char)0x80):
            {
                char* history_cmd_ptr=history_get_prev(&cmd_history);
                if(history_cmd_ptr!=NULL){
                    strcpy(buf,history_cmd_ptr);
                    while(pos!=buf){
                        putchar('\b');
                        pos--;
                    }
                    for(int i=0;i<strlen(history_cmd_ptr);i++){
                        putchar(history_cmd_ptr[i]);
                        pos++;
                    }
                }
                break;
            }
            case ((char)0x81):
            {
                char* history_cmd_ptr=history_get_next(&cmd_history);
                if(history_cmd_ptr!=NULL){
                    strcpy(buf,history_cmd_ptr);
                    while(pos!=buf){
                        putchar('\b');
                        pos--;
                    }
                    for(int i=0;i<strlen(history_cmd_ptr);i++){
                        putchar(history_cmd_ptr[i]);
                        pos++;
                    }
                }
                break;
            }
            default:
            {
                putchar(*pos);
                pos++;
            }
        }
    }
}

static void modify_cwd_cache(const char *filepath)
{
    ;
}

static void ls(void)
{
    sys_dir_list(cwd_cache);
}

static void lsi(void)
{
    sys_dir_list_info(cwd_cache);
}

static void ps(void)
{
    struct list_elem *elem = thread_all_list.head.next;
    while (elem->next != NULL)
    {
        struct task_struct *pcb = elem2entry(struct task_struct, all_list_tag, elem);
        char status_str[40];
        switch (pcb->status)
        {
        case TASK_RUNNING:
            strcpy(status_str, "TASK_RUNNING");
            break;
        case TASK_READY:
            strcpy(status_str, "TASK_READY");
            break;
        case TASK_BLOCKED:
            strcpy(status_str, "TASK_BLOCKED");
            break;
        case TASK_WAITING:
            strcpy(status_str, "TASK_WAITING");
            break;
        case TASK_HANGING:
            strcpy(status_str, "TASK_HANGING");
            break;
        case TASK_DIED:
            strcpy(status_str, "TASK_DIED");
            break;
        default:
            strcpy(status_str, "UNKNOWN_STATUS");
            break;
        }

        printf("pid %d, status %s, pname %s,ticks %d\n", pcb->pid, status_str, pcb->name, pcb->elapsed_ticks);
        elem = elem->next;
    }
}

static void process_program()
{
    // argv需要特殊处理
    char argv[5][128];
    int i = 4;
    while (cmd_line_exec_bat[i] == ' ')
    {
        i++;
    }
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    // 将当前工作目录复制到缓冲区
    strcpy(buf, cwd_cache);
    idx = strlen(buf);
    if (buf[idx - 1] != '/')
    {
        buf[idx++] = '/';
    }
    // 处理路径中的每个字符
    while (cmd_line_exec_bat[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
    {
        buf[idx++] = cmd_line_exec_bat[i++];
    }
    strcpy(argv[0], buf);
    if (get_file_type(argv[0]) != FT_REGULAR)
    {
        printf("%s No such file or directory\n", argv[0]);
    }
    else
    {
        execv(argv[0], argv);
    }
    process_exit();
}

static void process_rm_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 2; // 假设命令行从 'rm' 后开始
    // 跳过多余的空格
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
    {
        i++;
    }

    // 解析路径，直到遇到无效字符或者超出最大长度
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    // 将当前工作目录复制到缓冲区
    strcpy(buf, cwd_cache);
    idx = strlen(buf);
    if (buf[idx - 1] != '/')
    {
        buf[idx++] = '/';
    }

    // 处理路径中的每个字符
    while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
    {
        buf[idx++] = cmd_line[i++];
    }

    buf[idx] = '\0'; // 终止字符串
    // 检查文件类型
    if (get_file_type(buf) != FT_REGULAR)
    {
        printf("Invalid file: %s\n", buf);
        return;
    }

    // 删除文件
    if (delete (buf) != -1)
    {
        printf("File removed: %s\n", buf);
    }
    else
    {
        printf("Failed to remove file: %s\n", buf);
    }
}

static void process_touch_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 5;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: touch <filename>\n\n");
        return;
    }
    char buf[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    strcpy(buf, cwd_cache);
    idx = strlen(buf);
    if (buf[idx - 1] != '/')
        buf[idx++] = '/';
    while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        buf[idx++] = cmd_line[i++];
    buf[idx] = '\0';
    int type = get_file_type(buf);
    if (type == FT_REGULAR || type == FT_DIRECTORY || type == FT_UNKNOWN)
    {
        printf("File already exists: %s\n", buf);
        return;
    }
    int fd = openFile(buf, O_CREAT);
}

static void process_edit_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 4;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: edit <filename>\n\n");
        return;
    }
    char filename[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    while (cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        filename[idx++] = cmd_line[i++];
    filename[idx] = '\0';
    char filepath[MAX_PATH_LENGTH] = {0};
    strcpy(filepath, cwd_cache);
    idx = strlen(filepath);
    if (filepath[idx - 1] != '/')
    {
        filepath[idx++] = '/';
        filepath[idx] = '\0';
    }
    strcat(filepath, filename);
    int type = get_file_type(filepath);
    if (type != FT_REGULAR)
    {
        printf("Invalid file: %s\n", filepath);
        return;
    }
    editor_main(filepath);
}

void my_shell(void)
{
    history_init(&cmd_history);
    cwd_cache[0]='/';
    while(1){
        sema_init(&(running_thread()->waiting_sema),0);
        print_prompt();
        memset(cmd_line, 0, cmd_len);
        readline(cmd_line, cmd_len);
        if (cmd_line[0] == 0)
        {
            continue;
        }
        if (cmd_line[0] == 'l' && cmd_line[1] == 's' && cmd_line[2] == 'i')
        {
            thread_start("lsi", SECOND_PRIO, lsi, NULL);
            thread_wait();
        }
        if (cmd_line[0] == 'l' && cmd_line[1] == 's')
        {
            thread_start("ls", SECOND_PRIO, ls, NULL);
            thread_wait();
        }
        if (cmd_line[0] == 'p' && cmd_line[1] == 's')
        {
            thread_start("ps", SECOND_PRIO, ps, NULL);
            thread_wait();
        }
        if (cmd_line[0] == 'c' && cmd_line[1] == 'd')
        {
            process_cd_command(cmd_line);
        }
        if (cmd_line[0] == 'm' && cmd_line[1] == 'k' && cmd_line[2] == 'd' && cmd_line[3] == 'i' && cmd_line[4] == 'r')
        {
            strcpy(cmd_line_ps_bat, cmd_line);
            thread_start("mkdir", SECOND_PRIO, process_mkdir_command, (cmd_line_ps_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'k' && cmd_line[1] == 'i' && cmd_line[2] == 'l' && cmd_line[3] == 'l')
        {
            strcpy(cmd_line_kill_bat, cmd_line);
            thread_start("kill", SECOND_PRIO, process_kill_command, (cmd_line_kill_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'c' && cmd_line[1] == 'a' && cmd_line[2] == 't')
        {
            strcpy(cmd_line_cat_bat, cmd_line);
            thread_start("cat", SECOND_PRIO, process_cat_command, (cmd_line_cat_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'd' && cmd_line[1] == 'b' && cmd_line[2] == 'g')
        {
            if (cmd_line[4] == 'f' && cmd_line[5] == 't')
            {
                for (int i = 0; i < MAX_FILE_OPEN; i += 2)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        printf("GFT[%d]:status:%s ", i + j, file_table[i + j].fd_inode != NULL ? "OPENED" : "EMPTY ");
                    }
                    printf("\n");
                }
            }
            if (cmd_line[4] == 'n' && cmd_line[5] == 'd')
            {
                struct list_elem *elem = cur_part->open_inodes.head.next;
                while (elem->next != NULL)
                {
                    struct inode *Inode = elem2entry(struct inode, inode_tag, elem);
                    printf("opened_inode:%d ", Inode->i_no);
                    elem = elem->next;
                }
                printf("\n");
            }
        }
        if(cmd_line[0]=='r'&&cmd_line[1]=='m'){
            strcpy(cmd_line_rm_bat,cmd_line);
            thread_start("rm", SECOND_PRIO, process_rm_command, (cmd_line_rm_bat));
            thread_wait();
        }
        if(cmd_line[0]=='e'&&cmd_line[1]=='x'&&cmd_line[2]=='e'&&cmd_line[3]=='c'){
            strcpy(cmd_line_exec_bat,cmd_line);
            process_execute(((uint32_t)process_program),"loader");
        }
        if (cmd_line[0] == 'r' && cmd_line[1] == 'm')
        {
            strcpy(cmd_line_rm_bat, cmd_line);
            thread_start("rm", SECOND_PRIO, process_rm_command, (cmd_line_rm_bat));
            thread_wait();
        }
        if (cmd_line[0] == 't' && cmd_line[1] == 'o' && cmd_line[2] == 'u' && cmd_line[3] == 'c' && cmd_line[4] == 'h')
        {
            strcpy(cmd_line_touch_bat, cmd_line);
            thread_start("touch", SECOND_PRIO, process_touch_command, (cmd_line_touch_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'e' && cmd_line[1] == 'd' && cmd_line[2] == 'i' && cmd_line[3] == 't')
        {
            strcpy(cmd_line_edit_bat, cmd_line);
            thread_start("edit", SECOND_PRIO, process_edit_command, (cmd_line_edit_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'c' && cmd_line[1] == 'l' && cmd_line[2] == 'e' && cmd_line[3] == 'a' && cmd_line[4] == 'r')
        {
            for (int i = 0; i < 25 * 80; i++)
                printf(" ");
            set_cursor(0);
        }
        history_push(&cmd_history, cmd_line);
    }
}