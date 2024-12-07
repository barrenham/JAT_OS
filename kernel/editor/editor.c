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
#include "../include/interrupt.h"

#define BUF_SIZE (70 * 1024)

struct editor_buf
{
    uint32_t size;
    uint32_t capacity;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t pos;
    int display_start_line;
    int display_end_line;
    uint32_t line_count;
    uint32_t *line_offsets;
    bool edit_mode;
    char filename[128];
    char *content;
};

static void update_pos(struct editor_buf *buf)
{
    uint32_t line = buf->cursor_y + buf->display_start_line;
    if (line >= buf->line_count)
        line = buf->line_count - 1;
    uint32_t line_start = buf->line_offsets[line];
    uint32_t line_end;
    if (line + 1 < buf->line_count)
        line_end = buf->line_offsets[line + 1] - 1;
    else
        line_end = buf->size;
    uint32_t x = buf->cursor_x;
    uint32_t line_length = line_end - line_start;
    if (x > line_length)
        x = line_length;
    buf->pos = line_start + x;
}

void split_lines(struct editor_buf *buf)
{
    buf->line_count = 0;
    free(buf->line_offsets);
    buf->line_offsets = malloc(sizeof(uint32_t) * 10000);
    if (!buf->line_offsets)
    {
        printf("Failed to allocate memory for line offsets.\n");
        return;
    }
    buf->line_offsets[buf->line_count++] = 0;
    uint32_t current_line_length = 0;
    for (uint32_t i = 0; i < buf->size; i++)
    {
        if (buf->content[i] == '\n')
        {
            if (buf->line_count < 10000)
            {
                buf->line_offsets[buf->line_count++] = i + 1;
                current_line_length = 0;
            }
            else
            {
                printf("Exceeded maximum number of lines.\n");
                break;
            }
        }
        else
        {
            current_line_length++;
            if (current_line_length >= 80)
            {
                if (buf->line_count < 10000)
                {
                    buf->line_offsets[buf->line_count++] = i + 1;
                    current_line_length = 0;
                }
                else
                {
                    printf("Exceeded maximum number of lines.\n");
                    break;
                }
            }
        }
    }
}

void display_content(struct editor_buf *buf)
{
    uint32_t current_line = buf->display_start_line;
    uint32_t end_line = buf->display_end_line;
    if (end_line > buf->line_count)
        end_line = buf->line_count;
    console_acquire();
    clean_screen();
    set_cursor(0);
    console_release();
    for (uint32_t line_num = current_line; line_num < end_line; line_num++)
    {
        uint32_t line_start = buf->line_offsets[line_num];
        uint32_t line_end;
        if (line_num + 1 < buf->line_count)
            line_end = buf->line_offsets[line_num + 1] - 1;
        else
            line_end = buf->size;
        for (uint32_t i = line_start; i < line_end; i++)
        {
            if (buf->content[i] == '\0' || buf->content[i] == '\b')
                printf(" ");
            else
                printf("%c", buf->content[i]);
        }
        printf("\n");
    }
    console_acquire();
    set_cursor(80 * 22);
    console_release();
    printf("-------------------------------------------------\n\n");
    printf("File: %s %s %d\n", buf->filename, buf->edit_mode ? "-- INSERT --" : "-- NORMAL --", buf->size);
    printf("Line: %d/%d ", buf->cursor_y + buf->display_start_line + 1, buf->line_count);
}

static void update_cursor_position(struct editor_buf *buf)
{
    uint32_t current_line = buf->cursor_y + buf->display_start_line;
    if (current_line >= buf->line_count)
    {
        current_line = buf->line_count - 1;
        buf->cursor_y = current_line - buf->display_start_line;
    }
    uint32_t line_start = buf->line_offsets[current_line];
    uint32_t line_end;
    if (current_line + 1 < buf->line_count)
        line_end = buf->line_offsets[current_line + 1] - 1;
    else
        line_end = buf->size;
    uint32_t line_length = line_end - line_start;
    if (buf->cursor_x > line_length)
        buf->cursor_x = line_length;
    if (buf->cursor_y >= 22)
    {
        if (buf->display_end_line < buf->line_count)
        {
            buf->display_start_line++;
            buf->display_end_line++;
            buf->cursor_y = 21;
            display_content(buf);
        }
        else
            buf->cursor_y = 21;
    }
    else if (buf->cursor_y < 0)
    {
        if (buf->display_start_line > 0)
        {
            buf->display_start_line--;
            buf->display_end_line--;
            buf->cursor_y = 0;
            display_content(buf);
        }
        else
            buf->cursor_y = 0;
    }
    console_acquire();
    set_cursor(buf->cursor_y * 80 + buf->cursor_x);
    console_release();
}



static void insert_char(struct editor_buf *buf, char c)
{
    if (buf->size >= buf->capacity - 1)
        return;
    uint32_t pos = buf->pos;
    if (pos > buf->size)
        pos = buf->size;
    for (uint32_t i = buf->size; i > pos; i--)
        buf->content[i] = buf->content[i - 1];
    buf->content[pos] = c;
    buf->size++;
    buf->pos++;
    if (c == '\n')
    {
        buf->cursor_x = 0;
        buf->cursor_y++;
    }
    else
        buf->cursor_x++;
    split_lines(buf);
    if (buf->line_count <= 22)
        buf->display_end_line = buf->line_count;
    update_cursor_position(buf);
}

static void delete_char(struct editor_buf *buf)
{
    if (buf->size > 0 && buf->pos > 0)
    {
        uint32_t pos = buf->pos - 1;
        char deleted_char = buf->content[pos];
        for (uint32_t i = pos; i < buf->size - 1; i++)
            buf->content[i] = buf->content[i + 1];
        buf->size--;
        buf->pos--;
        if (deleted_char == '\n')
        {
            if (buf->cursor_y > 0)
            {
                buf->cursor_y--;
                uint32_t current_line = buf->cursor_y + buf->display_start_line;
                if (current_line >= buf->line_count)
                    current_line = buf->line_count - 1;
                uint32_t line_start = buf->line_offsets[current_line];
                uint32_t line_end;
                if (current_line + 1 < buf->line_count)
                    line_end = buf->line_offsets[current_line + 1] - 1;
                else
                    line_end = buf->size;
                uint32_t line_length = line_end - line_start;
                buf->cursor_x = line_length;
            }
            else
            {
                if (buf->display_start_line > 0)
                {
                    buf->display_start_line--;
                    buf->display_end_line--;
                    uint32_t current_line = buf->cursor_y + buf->display_start_line;
                    if (current_line >= buf->line_count)
                        current_line = buf->line_count - 1;
                    uint32_t line_start = buf->line_offsets[current_line];
                    uint32_t line_end;
                    if (current_line + 1 < buf->line_count)
                        line_end = buf->line_offsets[current_line + 1] - 1;
                    else
                        line_end = buf->size;
                    uint32_t line_length = line_end - line_start;
                    buf->cursor_x = line_length;
                }
            }
        }
        else
        {
            if (buf->cursor_x > 0)
                buf->cursor_x--;
        }
        split_lines(buf);
        if (buf->line_count <= 22)
            buf->display_end_line = buf->line_count;
        update_cursor_position(buf);
    }
}

void editor_main(const char *filename)
{
    console_acquire();
    clean_screen();
    console_release();
    struct editor_buf buf;
    buf.content = malloc(BUF_SIZE);
    if(buf.content==NULL){
        return;
    }
    memset(buf.content, 0, BUF_SIZE);
    buf.capacity = BUF_SIZE;
    buf.size = 0;
    buf.cursor_x = 0;
    buf.cursor_y = 0;
    buf.edit_mode = False;
    strcpy(buf.filename, filename);
    int fd = openFile(filename, O_RDWR);
    filesize size = getfilesize(fd);
    if (fd != -1)
    {
        seekp(fd, 0, SEEK_SET);
        int read_cnt = read(fd, buf.content, BUF_SIZE);
        buf.size = size;
        closeFile(fd);
    }
    else
    {
        printf("Failed to open file: %s\n", filename);
        return;
    }
    if (buf.size > 0 && buf.content[buf.size - 1] == '\0')
        buf.size--;
    buf.line_offsets = malloc(sizeof(uint32_t) * 10000);
    split_lines(&buf);
    buf.display_start_line = 0;
    buf.display_end_line = buf.line_count > 22 ? 22 : buf.line_count;
    int size1 = buf.size;
    buf.pos = 0;
    bool first_open = True;
    bool press_up = False;
    bool press_down = False;
    while (1)
    {
        console_acquire();
        clean_screen();
        set_cursor(0);
        console_release();
        display_content(&buf);
        if (first_open)
        {
            console_acquire();
            set_cursor(0);
            console_release();
            first_open = False;
        }
        else{
            console_acquire();
            set_cursor(buf.cursor_y * 80 + buf.cursor_x);
            console_release();
        }     
        char c = getchar();
        if (!buf.edit_mode)
        {
            int bufsize;
            if (c == 'i')
                buf.edit_mode = True;
            else if (c == 's')
            {
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
            }
            else if (c == 'q')
            {
                console_acquire();
                clean_screen();
                set_cursor(0);
                console_release();
                free(buf.line_offsets);
                free(buf.content);
                return;
            }
            else if (c == ((char)0x80))
            {
                if (buf.cursor_y > 0)
                {
                    buf.cursor_y--;
                    uint32_t line = buf.cursor_y + buf.display_start_line;
                    uint32_t line_start = buf.line_offsets[line];
                    uint32_t line_end = (line + 1 < buf.line_count) ? buf.line_offsets[line + 1] - 1 : buf.size;
                    uint32_t line_length = line_end - line_start;
                    if (buf.cursor_x > line_length)
                        buf.cursor_x = line_length;
                    update_pos(&buf);
                    update_cursor_position(&buf);
                }
                else
                {
                    if (buf.display_start_line > 0)
                    {
                        buf.display_start_line--;
                        buf.display_end_line--;
                        uint32_t line = buf.cursor_y + buf.display_start_line;
                        uint32_t line_start = buf.line_offsets[line];
                        uint32_t line_end = (line + 1 < buf.line_count) ? buf.line_offsets[line + 1] - 1 : buf.size;
                        uint32_t line_length = line_end - line_start;
                        if (buf.cursor_x > line_length)
                            buf.cursor_x = line_length;
                        update_pos(&buf);
                        display_content(&buf);
                    }
                }
            }
            else if (c == ((char)0x81))
            {
                if (buf.cursor_y < 21)
                {
                    if (buf.cursor_y + buf.display_start_line + 1 < buf.line_count)
                    {
                        buf.cursor_y++;
                        uint32_t line = buf.cursor_y + buf.display_start_line;
                        uint32_t line_start = buf.line_offsets[line];
                        uint32_t line_end = (line + 1 < buf.line_count) ? buf.line_offsets[line + 1] - 1 : buf.size;
                        uint32_t line_length = line_end - line_start;
                        if (buf.cursor_x > line_length)
                            buf.cursor_x = line_length;
                        update_pos(&buf);
                        update_cursor_position(&buf);
                    }
                }
                else
                {
                    if (buf.display_end_line < buf.line_count)
                    {
                        buf.display_start_line++;
                        buf.display_end_line++;
                        uint32_t line = buf.cursor_y + buf.display_start_line;
                        uint32_t line_start = buf.line_offsets[line];
                        uint32_t line_end = (line + 1 < buf.line_count) ? buf.line_offsets[line + 1] - 1 : buf.size;
                        uint32_t line_length = line_end - line_start;
                        if (buf.cursor_x > line_length)
                            buf.cursor_x = line_length;
                        update_pos(&buf);
                        display_content(&buf);
                    }
                }
            }
            else if (c == '<')
            {
                if (buf.cursor_x > 0)
                {
                    buf.cursor_x--;
                    buf.pos--;
                }
                update_cursor_position(&buf);
            }
            else if (c == '>')
            {
                if (buf.pos < buf.size)
                {
                    buf.cursor_x++;
                    if (buf.cursor_x >= 80)
                        buf.cursor_x = 79;
                    if (buf.content[buf.pos] != '\n')
                        buf.pos++;
                    else
                    {
                        buf.cursor_x = 0;
                        buf.cursor_y++;
                        buf.pos++;
                    }
                }
                update_cursor_position(&buf);
            }
        }
        else
        {
            if (c == 27)
                buf.edit_mode = False;
            else if (c == '\b')
                delete_char(&buf);
            else
                insert_char(&buf, c);
        }
    }
}