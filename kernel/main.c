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
	printf("hello hello hello\n",NULL);
	uint32_t ptr=0;
	uint32_t read_cnt=0;
	while((read_cnt=sys_read(fd0,buf,ptr,0x200))!=-1){
		ptr+=read_cnt;
		for(int i=0;i<(read_cnt);i++){
			printf("%c",buf[i],NULL);
		}
	}
	free(buf);
	process_execute(u_prog_c, "test_write_file");
	process_execute(u_prog_b, "u_prog_b"); 
	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 31, k_thread_b, "argB ");  
	process_execute(u_prog_a, "u_prog_a");
	void* addr1=malloc_page(PF_KERNEL,2);
	void* addr2=malloc_page(PF_KERNEL,1);
	console_put_string(" ADDR1 malloc addr:0x");
	console_put_int(addr1);
	console_put_string(" ADDR2 malloc addr:0x");
	console_put_int(addr2);
	mfree_page(PF_KERNEL,addr1,2);
	mfree_page(PF_KERNEL,addr2,1);
	printf("\n",NULL);
	sys_close(fd0);
	while(1){
		printf("I'm thread main\n",NULL);
		int cpu_delay = 10000000;
		while(cpu_delay-->0);
	}
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
	int cpu_delay = 100000;
	while(cpu_delay-->0);
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	while(1){
		printf("I'm thread 1\n",NULL);
		int cpu_delay = 10000000;
		while(cpu_delay-->0);
	}
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
	int cpu_delay = 100000;
	while(cpu_delay-->0);
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	while(1){
		printf("I'm thread 2\n",NULL);
		int cpu_delay = 10000000;
		while(cpu_delay-->0);
	}
}

void u_prog_a(void) {
	//printf("hello, something has occured!\n",NULL);
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);
	printf(" prog_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
	int cpu_delay = 100000;
	while(cpu_delay-->0);
	free(addr1);
	free(addr2);
	free(addr3);
	while(1){
		printf("I'm program 1\n",NULL);
		int cpu_delay = 10000000;
		while(cpu_delay-->0);
	}
}

void u_prog_b(void) {
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);
	printf(" prog_b malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
	int cpu_delay = 100000;
	while(cpu_delay-->0);
	free(addr1);
	free(addr2);
	free(addr3);
	while(1){
		printf("I'm program 2\n",NULL);
		int cpu_delay = 10000000;
		while(cpu_delay-->0);
	}
}

void u_prog_c(void){
	printf("I'm test_write_file\n",NULL);
	int32_t fd=openFile("/hello.txt",O_RDWR);
	for(int i=0;i<10;i++){
		write(fd,"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",60);
		printf("%d  ",i);
	}
	seekp(fd,0,SEEK_SET);
	char buf[60];
	while(read(fd,buf,60)>=0){
		printf("%s",buf,NULL);
	}
	while(1);
}