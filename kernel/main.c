#include "include/stdio.h"
#include "include/print.h"
#include "include/console.h"
#include "include/init.h"
#include "include/debug.h"
#include "include/string.h"
#include "include/memory.h"
#include "include/thread.h"
#include "include/interrupt.h"
#include "include/console.h"
#include "include/ioqueue.h"
#include "include/keyboard.h"
#include "include/process.h"
#include "include/syscall.h"
#include "include/fs.h"
#include "include/shell.h"
#include "include/log.h"
#include "include/stdio-kernel.h"

extern struct ide_channel channels[2];

void k_thread_a(void* );
void k_thread_b(void* );
void u_prog_a (void);
void u_prog_b (void);
void u_prog_c (void);


void serveProcess(void);
                                          

int main(void) {
	put_string("I am kernel\n");
	init_all();
	intr_enable();
	char* buf=(char*)sys_malloc(0x200);
	int fd0=sys_open("/welcome.txt",O_RDWR);
	int cnt=0;
	console_put_char('\n');
	printf("hello hello hello");
	putchar('\n');
	uint32_t ptr=0;
	uint32_t read_cnt=0;
	
	while((read_cnt=sys_read(fd0,buf,ptr,0x200))!=-1){
		ptr+=read_cnt;
		for(int i=0;i<(read_cnt);i++){
			printf("%c",buf[i]);
		}
	}
	free(buf);
	sys_close(fd0);
	{
		uint32_t file_size=100*512;
		void* prog_buf=get_kernel_pages(25);
		uint32_t sec_cnt=DIV_ROUND_UP(file_size,512);
		struct disk* sda=&channels[0].devices[0];
		ide_read(sda,400,prog_buf,sec_cnt);
		int32_t fd=sys_open("/highP",O_CREAT|O_RDWR);
		if(fd!=-1){
			if(sys_write(fd,prog_buf,0,file_size)==-1){
				printk("file write error!\n");
			}
		}
		mfree_page(PF_KERNEL, prog_buf, 25);
		closeFile(fd);
	}
	
	thread_start("thread_cleaner",FIRST_PRIO,thread_cleaner,NULL);
	//process_execute(u_prog_a, "u_prog_a");
	//process_execute(u_prog_c, "test_write_file");
	// process_execute(u_prog_b, "u_lazy"); 
	/*
	thread_start("k_thread_a", FIRST_PRIO, k_thread_a, "argA ");
	thread_start("k_thread_b", FIRST_PRIO, k_thread_b, "argB ");  
	void* addr1=malloc_page(PF_KERNEL,2);
	void* addr2=malloc_page(PF_KERNEL,1);
	console_put_string(" ADDR1 malloc addr:0x");
	console_put_int(addr1);
	console_put_string(" ADDR2 malloc addr:0x");
	console_put_int(addr2);
	mfree_page(PF_KERNEL,addr1,2);
	mfree_page(PF_KERNEL,addr2,1);
	printf("\n");
	char* lptr=sys_malloc(102400);
	sys_free(lptr);
	*/
	logEnable();
	thread_start("shell",FIRST_PRIO,my_shell,NULL);
	//process_execute(my_shell,"shell");
	while(1);
	return 0;
}



void k_thread_a(void* arg){
	console_put_char('a');
	void* addr1 = sys_malloc(256);
	void* addr2 = sys_malloc(255);
	void* addr3 = sys_malloc(254);
	console_put_string(" thread_a malloc addr:0x");
	console_put_int((int)addr1);
	console_put_char(',');
	console_put_int((int)addr2);
	console_put_char(',');
	console_put_int((int)addr3);
	console_put_char('\n');
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	printf("I'm thread 1 ");
	int cpu_delay = 1000000;
	while(cpu_delay-->0);
}

void k_thread_b(void* arg){
	console_put_char('b');
	void* addr1 = sys_malloc(256);
	void* addr2 = sys_malloc(255);
	void* addr3 = sys_malloc(254);
	console_put_string(" thread_b malloc addr:0x");
	console_put_int((int)addr1);
	console_put_char(',');
	console_put_int((int)addr2);
	console_put_char(',');
	console_put_int((int)addr3);
	console_put_char('\n');
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	printf("I'm thread 2 ");
	int cpu_delay = 1000000;
	while(cpu_delay-->0);
}

void u_prog_a(void) {
	if(fork()==0){
		process_exit();
	}
	if(fork()==0){
		process_exit();
	}
	int fd[2];
	pipe(fd);
	write(fd[0],"hello, child!\n",15);
	if(fork()==0){
		char buf[30]={0};
		read(fd[1],buf,30);
		printf("%s",buf);
		process_exit();
	}
	process_exit();
}

void u_prog_b(void) {
	while(1);
	process_exit();
}

void u_prog_c(void){
	printf("I'm test_write_file\n");
	/*
	int32_t fd=openFile("/hello.txt",O_RDWR);
	seekp(fd,0,SEEK_SET);
	*/
	char buf[200];
	int read_cnt=0;
	int32_t fd=openFile("/hello.txt",O_RDONLY);
	printf("filesize:%d B\n",getfilesize(fd));
	seekp(fd,2000,SEEK_SET);
	remove_some_cotent(fd,70000);
	seekp(fd,0,SEEK_SET);
	while((read_cnt=read(fd,buf,200))>=0){
		for(int i=0;i<read_cnt;i++){
			printf("%c",buf[i]);
		}
	}
	//closeFile(fd);
	process_exit();
}