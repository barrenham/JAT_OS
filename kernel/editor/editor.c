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
struct editor_buf
{
    char *content;
    uint32_t size;
    uint32_t capacity;
    uint32_t cursor_x;
    uint32_t cursor_y;
    bool edit_mode;
    char filename[128];
};

static void update_cursor_position(struct editor_buf *buf)
{
    if (buf->cursor_x >= 80)
    {
        buf->cursor_x = 0;
        buf->cursor_y++;
    }
    if (buf->cursor_y >= 25)
    {
        buf->cursor_y = 24;
    }
    set_cursor(buf->cursor_y * 80 + buf->cursor_x);
}

static uint32_t get_line_length(struct editor_buf *buf, uint32_t line_start)
{
    uint32_t len = 0;
    while (line_start + len < buf->size && buf->content[line_start + len] != '\n')
    {
        len++;
    }
    return len;
}

static void delete_char(struct editor_buf *buf)
{
    if (buf->size > 0 && (buf->cursor_y * 80 + buf->cursor_x) > 0)
    {
        uint32_t pos = buf->cursor_y * 80 + buf->cursor_x - 1;
        for (uint32_t i = pos; i < buf->size - 1; i++)
        {
            buf->content[i] = buf->content[i + 1];
        }
        buf->content[buf->size - 1] = '\0';
        buf->size--;
        if (buf->cursor_x > 0)
        {
            buf->cursor_x--;
        }
        else if (buf->cursor_y > 0)
        {
            buf->cursor_y--;
            uint32_t line_start = buf->cursor_y * 80;
            buf->cursor_x = get_line_length(buf, line_start);
        }
        update_cursor_position(buf);
    }
}

static void insert_char(struct editor_buf *buf, char c)
{
    if (buf->size >= buf->capacity - 1)
        return;
    uint32_t pos = buf->cursor_y * 80 + buf->cursor_x;
    if (pos > buf->size)
        pos = buf->size;
    for (uint32_t i = buf->size; i > pos; i--)
        buf->content[i] = buf->content[i - 1];
    buf->content[pos] = c;
    buf->size++;
    if (c == '\n')
    {
        buf->cursor_x = 0;
        buf->cursor_y++;
    }
    else
    {
        buf->cursor_x++;
        if (buf->cursor_x >= 80)
        {
            buf->cursor_x = 0;
            buf->cursor_y++;
        }
    }
    update_cursor_position(buf);
}

static void clear_screen()
{
    for (int i = 0; i < 25; i++)
        printf("\n");
    set_cursor(0);
}

void editor_main(const char *filename)
{
    struct editor_buf buf;
    buf.content = malloc(BUF_SIZE);
    memset(buf.content, 0, BUF_SIZE);
    buf.capacity = BUF_SIZE;
    buf.size = 0;
    buf.cursor_x = 0;
    buf.cursor_y = 2;
    set_cursor(buf.cursor_y * 80 + buf.cursor_x);
    buf.edit_mode = False;
    strcpy(buf.filename, filename);
    int fd = openFile(filename, O_RDWR);
    filesize size = getfilesize(fd);
    if (fd != -1)
    {
        seekp(fd, 0, SEEK_SET);
        int read_cnt = read(fd, buf.content, BUF_SIZE);
        buf.size = read_cnt;
        closeFile(fd);
    }
    int size1 = buf.size;
    while (1)
    {
        clear_screen();
        printf("File: %s %s\n", buf.filename, buf.edit_mode ? "-- INSERT --" : "-- NORMAL --");
        printf("------------------------------------------------\n\n");
        for (uint32_t i = 0; i < buf.size; i++)
        {
            if (buf.content[i] == '\n')
            {
                printf("%c", buf.content[i]);
                buf.cursor_x = 0;
                buf.cursor_y++;
            }
            else
            {
                printf("%c", buf.content[i]);
                buf.cursor_x++;
                if (buf.cursor_x >= 80)
                {
                    buf.cursor_x = 0;
                    buf.cursor_y++;
                }
            }
        }
        if (buf.cursor_y >= 25)
            buf.cursor_y = 24;
        set_cursor(buf.cursor_y * 80 + buf.cursor_x);
        char c = getchar();
        if (!buf.edit_mode)
        {
            int bufsize;
            switch (c)
            {
            case 'i':
                buf.edit_mode = True;
                break;
            case 's':
                bufsize = buf.size;
                printf("\n\nsize: %d, orignal size: %d, file size: %d, size - buf.size: %d\n", bufsize, size1, size, size - buf.size);
                fd = openFile(filename, O_RDWR);
                if (fd != -1)
                {
                    seekp(fd, 0, SEEK_SET);
                    write(fd, buf.content, buf.size);
                    if (size > buf.size)
                    {
                        seekp(fd, buf.size, SEEK_SET);
                        remove_some_cotent(fd, size - buf.size);
                    }
                    closeFile(fd);
                }
                break;
            case 'q':
                free(buf.content);
                return;
            case 72:
                if (buf.cursor_y > 0)
                    buf.cursor_y--;
                update_cursor_position(&buf);
                break;
            case 80:
                buf.cursor_y++;
                update_cursor_position(&buf);
                break;
            case '<':
                if (buf.cursor_x > 0)
                    buf.cursor_x--;
                update_cursor_position(&buf);
                break;
            case '>':
                buf.cursor_x++;
                if (buf.cursor_x >= 80)
                {
                    buf.cursor_x = 0;
                    buf.cursor_y++;
                }
                update_cursor_position(&buf);
                break;
            default:
            }
        }
        else
        {
            if (c == 27)
                buf.edit_mode = False;
            else if (c == '\b')
            {
                delete_char(&buf);
                update_cursor_position(&buf);
            }
            else
                insert_char(&buf, c);
        }
    }
}