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
#include "../include/exec.h"
#include "../include/editor.h"
#include "../include/print.h"
#include "../include/log.h"
#include "../include/crypto.h"

#define cmd_len 128
#define MAX_ARG_NR 16
#define MAX_CMD_LENGTH cmd_len
#define HASH_SIZE 32

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
static char cmd_line_cp_bat[cmd_len] = {0};
static char cmd_line_rmdir_bat[cmd_len] = {0};
static char cmd_line_enc_bat[cmd_len] = {0};
static char cmd_line_dec_bat[cmd_len] = {0};
static char cmd_line_sha_bat[cmd_len] = {0};
static struct history cmd_history;

static bool isChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool isDigit(char c)
{
    return (c >= '0' && c <= '9');
}

char cwd_cache[64] = {0};

void print_prompt(void)
{
    printf("[user&localhost %s]$ ", cwd_cache);
}

static void process_rmdir_command(void* _cmd_line){
    char *cmd_line = _cmd_line;
    int i=5;
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
    if (get_file_type(buf) != FT_DIRECTORY)
    {
        printf("Not a directory: %s\n", buf);
        return;
    }
    if (delete (buf) != -1)
    {
        printf("Dir removed: %s\n", buf);
    }
    else
    {
        printf("Failed to remove dir: %s\n", buf);
    }
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
        printf("Not a file: %s\n", buf);
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

static void ls(void* args)
{
    sys_dir_list(cwd_cache);
}

static void lsi(void* args)
{
    sys_dir_list_info(cwd_cache);
}

static void ps(void* args)
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

static int verify_program_hash(const char *filepath)
{
    int fd = openFile(filepath, O_RDONLY);
    if (fd < 0)
        return -1;
    filesize size = getfilesize(fd);
    if (size <= HASH_SIZE)
    {
        closeFile(fd);
        return -1;
    }
    char stored_hash[HASH_SIZE];
    seekp(fd, size - HASH_SIZE, SEEK_SET);
    if (read(fd, stored_hash, HASH_SIZE) != HASH_SIZE)
    {
        closeFile(fd);
        return -1;
    }
    closeFile(fd);
    char current_hash[HASH_SIZE];
    SHA256(filepath, current_hash);
    return memcmp(stored_hash, current_hash, HASH_SIZE) == 0;
}

static int append_hash_to_program(const char *filepath)
{
    char current_hash[HASH_SIZE];
    SHA256(filepath, current_hash);
    int fd = openFile(filepath, O_RDWR);
    seekp(fd, 0, SEEK_END);
    if (write(fd, current_hash, HASH_SIZE) != HASH_SIZE)
    {
        closeFile(fd);
        return -1;
    }
    int flag = get_file_attr(fd);
    flag = flag | INODE_HASHED;
    set_file_attr(fd, flag);
    closeFile(fd);
    return 0;
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
        printk("%s No such file or directory\n", argv[0]);
    }
    else
    {
        int fd = openFile(argv[0], O_RDONLY);
        int flag = get_file_attr(fd);
        closeFile(fd);
        if (flag & INODE_HASHED)
        {
            if (!verify_program_hash(argv[0]))
            {
                printf("%s: hash verification failed\n", argv[0]);
                process_exit();
            }
        }
        else
        {
            if (append_hash_to_program(argv[0]) < 0)
            {
                printk("%s: failed to append hash\n", argv[0]);
                process_exit();
            }
        }
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
    closeFile(fd);
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

static void process_cp_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 2;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: cp <source_file_path> <destination_file_path>\n\n");
        return;
    }
    char src_path[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        src_path[idx++] = cmd_line[i++];
    src_path[idx] = '\0';
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    char dst_path[MAX_PATH_LENGTH] = {0};
    idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        dst_path[idx++] = cmd_line[i++];
    dst_path[idx] = '\0';
    if (src_path[0] == '\0' || dst_path[0] == '\0')
    {
        printf("Usage: cp <source_file_path> <destination_file_path>\n\n");
        return;
    }
    int type_src = get_file_type(src_path);
    if (type_src != FT_REGULAR)
    {
        printf("Invalid source file: %s\n", src_path);
        return;
    }
    int type_dst = get_file_type(dst_path);
    if (type_dst == FT_DIRECTORY)
    {
        printf("Invalid destination file: %s, use file path\n", dst_path);
        return;
    }
    if (type_dst != FT_REGULAR && type_dst != FT_UNKNOWN && type_dst != FT_DIRECTORY)
    {
        int fd = openFile(dst_path, O_CREAT);
        closeFile(fd);
    }
    int fd_dst = openFile(dst_path, O_RDWR);
    int fd_src = openFile(src_path, O_RDWR);
    filesize size_src = getfilesize(fd_src);
    filesize size_dst = getfilesize(fd_dst);
    if (size_dst > 0)
    {
        closeFile(fd_dst);
        delete(dst_path);
        fd_dst = openFile(dst_path, O_CREAT);
        closeFile(fd_dst);
        fd_dst = openFile(dst_path, O_RDWR);
    }
    char* buf=malloc(size_src+100);
    seekp(fd_src, 0, SEEK_SET);
    if (read(fd_src, buf, size_src) != size_src)
    {
        printf("Failed to read file: %s\n", src_path);
        closeFile(fd_dst);
        closeFile(fd_src);
        free(buf);
        return;
    }
    seekp(fd_dst, 0, SEEK_SET);
    if (write(fd_dst, buf, size_src) != size_src)
    {
        printf("Failed to write file: %s\n", dst_path);
        closeFile(fd_dst);
        closeFile(fd_src);
        free(buf);
        return;
    }
    closeFile(fd_dst);
    closeFile(fd_src);
    printf("File copied successfully from %s to %s\n", src_path, dst_path);
    free(buf);
}

static void process_enc_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 3;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: enc <source_file_path> <key>\n\n");
        return;
    }
    char src_path[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        src_path[idx++] = cmd_line[i++];
    src_path[idx] = '\0';
    int file_path_len = strlen(src_path);
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    uint8_t key[16] = {0};
    idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0')
        key[idx++] = cmd_line[i++];
    key[idx] = '\0';
    int key_len = strlen(key);
    if (src_path[0] == '\0' || key[0] == '\0')
    {
        printf("Usage: enc <source_file_path> <key>\n\n");
        return;
    }
    uint32_t rk[32];
    SM4_KeySchedule(key, rk);
    int fd = openFile(src_path, O_RDWR);
    int flags = get_file_attr(fd);
    if (flags & INODE_ENCRYPTED)
    {
        printf("File already encrypted: %s\n", src_path);
        closeFile(fd);
        return;
    }
    filesize size = getfilesize(fd);
    char* buf = malloc(size + 100);
    seekp(fd, 0, SEEK_SET);
    if (read(fd, buf, size) != size)
    {
        printf("Failed to read file: %s\n", src_path);
        closeFile(fd);
        free(buf);
        return;
    }
    SM4_ECB_Encrypt(buf, size, size + 100, &size, rk);
    seekp(fd, 0, SEEK_SET);
    if (write(fd, buf, size) != size)
    {
        printf("Failed to write file: %s\n", src_path);
        closeFile(fd);
        free(buf);
        return;
    }
    flags |= INODE_ENCRYPTED;
    set_file_attr(fd, flags);
    closeFile(fd);
    int fd_keys = openFile("/keys", O_RDWR);
    if (fd_keys < 0)
    {
        printf("Failed to open keys file\n");
        free(buf);
        return;
    }
    char line_buf[MAX_PATH_LENGTH + 1 + MAX_KEY_LEN];
    bool found = False;
    filesize pos = 0;
    filesize keys_size = getfilesize(fd_keys);
    while (pos < keys_size)
    {
        seekp(fd_keys, pos, SEEK_SET);
        if (read(fd_keys, line_buf, MAX_PATH_LENGTH + 1 + MAX_KEY_LEN) != MAX_PATH_LENGTH + 1 + MAX_KEY_LEN)
            break;
        if (strcmp(line_buf, src_path) == 0)
        {
            found = True;
            seekp(fd_keys, pos, SEEK_SET);
            break;
        }
        pos += MAX_PATH_LENGTH + 1 + MAX_KEY_LEN;
    }
    char *key_buf = malloc(MAX_PATH_LENGTH + 1 + MAX_KEY_LEN);
    memset(key_buf, 0, MAX_PATH_LENGTH + 1 + MAX_KEY_LEN);
    for (int i = 0; i < file_path_len; i++)
        key_buf[i] = src_path[i];
    for (int i = MAX_PATH_LENGTH + 1, j = 0; i <= MAX_PATH_LENGTH + 1 + key_len && j < key_len; i++, j++)
        key_buf[i] = key[j];
    if (!found)
        seekp(fd_keys, 0, SEEK_END);
    write(fd_keys, key_buf, MAX_PATH_LENGTH + 1 + MAX_KEY_LEN);
    free(key_buf);
    closeFile(fd_keys);
    printf("File encrypted successfully: %s\n", src_path);
}

static void process_dec_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 3;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: dec <source_file_path> <key>\n\n");
        return;
    }
    char src_path[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        src_path[idx++] = cmd_line[i++];
    src_path[idx] = '\0';
    int path_len = strlen(src_path);
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    uint8_t key[16] = {0};
    idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0')
        key[idx++] = cmd_line[i++];
    key[idx] = '\0';
    int key_len = strlen(key);
    if (src_path[0] == '\0' || key[0] == '\0')
    {
        printf("Usage: dec <source_file_path> <key>\n\n");
        return;
    }
    uint32_t rk[32];
    SM4_KeySchedule(key, rk);
    int fd = openFile(src_path, O_RDWR);
    int flags = get_file_attr(fd);
    if (!(flags & INODE_ENCRYPTED))
    {
        printf("File not encrypted: %s\n", src_path);
        closeFile(fd);
        return;
    }
    int fd_keys = openFile("/keys", O_RDONLY);
    int size = getfilesize(fd_keys);
    char *keys_buf = malloc(size);
    seekp(fd_keys, 0, SEEK_SET);
    int j = 0;
    read(fd_keys, keys_buf, size);
    for (j = 0; j < size; j += MAX_PATH_LENGTH + 1 + MAX_KEY_LEN)
    {
        if (strcmp(keys_buf + j, src_path) == 0)
            break;
    }
    if (strcmp(keys_buf + j + MAX_PATH_LENGTH + 1, key) != 0)
    {
        printf("Invalid key\n");
        closeFile(fd);
        closeFile(fd_keys);
        free(keys_buf);
        return;
    }
    closeFile(fd_keys);
    filesize size_file = getfilesize(fd);
    char *buf = malloc(size + 100);
    seekp(fd, 0, SEEK_SET);
    if (read(fd, buf, size_file) != size_file)
    {
        printf("Failed to read file: %s\n", src_path);
        closeFile(fd);
        free(buf);
        return;
    }
    SM4_ECB_Decrypt(buf, size_file, &size_file, rk);
    seekp(fd, 0, SEEK_SET);
    if (write(fd, buf, size_file) != size_file)
    {
        printf("Failed to write file: %s\n", src_path);
        closeFile(fd);
        free(buf);
        return;
    }
    seekp(fd, size_file, SEEK_SET);
    remove_some_cotent(fd, 16);
    flags &= ~INODE_ENCRYPTED;
    set_file_attr(fd, flags);
    closeFile(fd);
    printf("File dncrypted successfully: %s\n", src_path);
}

static void process_sha_command(void *_cmd_line)
{
    char *cmd_line = _cmd_line;
    int i = 4;
    while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
        i++;
    if (cmd_line[i] == '\0')
    {
        printf("Usage: sha <source_file_path>\n\n");
        return;
    }
    char src_path[MAX_PATH_LENGTH] = {0};
    int idx = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
        src_path[idx++] = cmd_line[i++];
    src_path[idx] = '\0';
    if (src_path[0] == '\0')
    {
        printf("Usage: sha <source_file_path>\n\n");
        return;
    }
    int type = get_file_type(src_path);
    if (type != FT_REGULAR)
    {
        printf("Invalid file: %s\n", src_path);
        return;
    }
    char sha[32] = {0};
    printf("src_path: %s, addr: %x\n", src_path, src_path);
    SHA256(src_path, sha);
    printf("src_path: %s, addr: %x\n", src_path, src_path);
    printf("SHA256: %s\n", sha);
}

void my_shell(void *args)
{
    history_init(&cmd_history);
    cwd_cache[0] = '/';
    while (1)
    {
        sema_init(&(running_thread()->waiting_sema), 0);
        print_prompt();
        memset(cmd_line, 0, cmd_len);
        readline(cmd_line, cmd_len);
        log_printk(DEVICE, "input %s \n", cmd_line);
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
        if (cmd_line[0] == 'r' && cmd_line[1] == 'm' && cmd_line[2] != 'd')
        {
            strcpy(cmd_line_rm_bat, cmd_line);
            thread_start("rm", SECOND_PRIO, process_rm_command, (cmd_line_rm_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'r' && cmd_line[1] == 'm' && cmd_line[2] == 'd' && cmd_line[3] == 'i' && cmd_line[4] == 'r')
        {
            strcpy(cmd_line_rmdir_bat, cmd_line);
            thread_start("rmdir", SECOND_PRIO, process_rmdir_command, (cmd_line_rmdir_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'r' && cmd_line[1] == 'm')
        {
            strcpy(cmd_line_rm_bat, cmd_line);
            thread_start("rm", SECOND_PRIO, process_rm_command, (cmd_line_rm_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'e' && cmd_line[1] == 'x' && cmd_line[2] == 'e' && cmd_line[3] == 'c')
        {
            // logDisable();
            strcpy(cmd_line_exec_bat, cmd_line);
            process_execute((process_program), "loader");
            // thread_wait();
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
            clean_screen();
            set_cursor(0);
        }
        if (cmd_line[0] == 'c' && cmd_line[1] == 'p')
        {
            strcpy(cmd_line_cp_bat, cmd_line);
            thread_start("cp", SECOND_PRIO, process_cp_command, (cmd_line_cp_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'e' && cmd_line[1] == 'n' && cmd_line[2] == 'c')
        {
            strcpy(cmd_line_enc_bat, cmd_line);
            thread_start("enc", SECOND_PRIO, process_enc_command, (cmd_line_enc_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'd' && cmd_line[1] == 'e' && cmd_line[2] == 'c')
        {
            strcpy(cmd_line_dec_bat, cmd_line);
            thread_start("dec", SECOND_PRIO, process_dec_command, (cmd_line_dec_bat));
            thread_wait();
        }
        if (cmd_line[0] == 's' && cmd_line[1] == 'h' && cmd_line[2] == 'a')
        {
            strcpy(cmd_line_sha_bat, cmd_line);
            thread_start("sha", SECOND_PRIO, process_sha_command, (cmd_line_sha_bat));
            thread_wait();
        }
        if (cmd_line[0] == 'a' && cmd_line[1] == 't' && cmd_line[2] == 't' && cmd_line[3] == 'r')
        {
            int i = 5;
            while (cmd_line[i] == ' ' && i < MAX_CMD_LENGTH)
                i++;
            if (cmd_line[i] == '\0')
            {
                printf("Usage: attr <source_file_path>\n\n");
                return;
            }
            char src_path[MAX_PATH_LENGTH] = {0};
            int idx = 0;
            while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && idx < MAX_PATH_LENGTH - 1)
                src_path[idx++] = cmd_line[i++];
            src_path[idx] = '\0';
            if (src_path[0] == '\0')
            {
                printf("Usage: attr <source_file_path>\n\n");
                return;
            }
            int fd = openFile(src_path, O_RDONLY);
            int flag = get_file_attr(fd);
            closeFile(fd);
            printf("flag %d\n", flag);
        }
        history_push(&cmd_history, cmd_line);
    }
}