#include "../include/editor.h"
#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/keyboard.h"
#include "../include/file.h"
#include "../include/syscall.h"
#include "../include/memory.h"
#include "../include/console.h"
#include "../include/stdint.h"
#include "../include/sync.h"
#include "../include/thread.h"
#include "../include/print.h"

#define BUF_SIZE (70 * 1024)

void editor_main(const char *filename)
{
    console_clear();
    char *buffer = (char *)malloc(BUF_SIZE);
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for editor.\n");
        return;
    }
    memset(buffer, 0, BUF_SIZE);
    file_descriptor fd = openFile(filename, O_RDWR);
    filesize file_size = getfilesize(fd);
    seekp(fd, 0, SEEK_SET);
    char *read_buf = (char *)malloc(BUF_SIZE);
    memset(read_buf, 0, BUF_SIZE);
    int read_cnt = 0;
    read_cnt = read(fd, read_buf, BUF_SIZE);
    for (int i = 0; i < read_cnt; i++)
        printf("%c", read_buf[i]);
    int cursor = file_size >= 0 ? file_size : 0;
    int row = 0, col = 0;
    for (int i = 0; i < cursor; i++)
    {
        if (read_buf[i] == '\n')
        {
            row++;
            col = 0;
        }
        else
            col++;
    }
    int pos = row * 80 + col;
    set_cursor(pos);
    char c;
    int32_t read_bytes;
    while (1)
    {
        read_bytes = read(stdin_no, &c, 1);
        if (read_bytes == -1)
        {
            printf("Error reading input.\n");
            break;
        }
        if (c == 27)
            break;
        else if (c == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                read_buf[cursor] = '\0';
                printf("\b \b");
            }
        }
        else
        {
            if (cursor < BUF_SIZE - 1)
            {
                read_buf[cursor++] = c;
                read_buf[cursor] = '\0';
                putchar(c);
            }
        }
    }
    seekp(fd, 0, SEEK_SET);
    write(fd, read_buf, cursor);
    closeFile(fd);
    free(buffer);
    free(read_buf);
    printf("\nFile saved: %s\n", filename);
}
