#ifndef LOG_H
#define LOG_H

#define LOG_PROCESS_FILE "/log_process.log"
#define LOG_ACESS_FILE   "/log_file.log"
#define LOG_SYSCALL_FILE "/log_syscall.log"
#define LOG_DEVICE_FILE  "/log_device.log"

enum log_status{
    log_disable,
    log_enable
};

enum log_type{
    PROCESS,
    FILE,
    SYSCALL,
    DEVICE
};

void log_init();
void log_acquire();
void log_release();
void log_put_string(char* str,enum log_type log_type);

enum log_status logEnable();
enum log_status logDisable();

enum log_status log_setstatus(enum log_status status);

enum log_status get_log_status();
#endif