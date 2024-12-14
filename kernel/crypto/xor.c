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

void encrypt_data(uint8_t *data, uint32_t size, const char *key)
{
    uint32_t key_len = strlen(key);
    for (uint32_t i = 0; i < size; i++)
        data[i] ^= key[i % key_len];
}

void decrypt_data(uint8_t *data, uint32_t size, const char *key)
{
    encrypt_data(data, size, key);
}