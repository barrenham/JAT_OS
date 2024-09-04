kernel.bin: main.o lib/*.o
	ld -m elf_i386 $^ -Ttext 0xc0001500 -e main -o kernel.bin
main.o:main.c
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector
lib/print.o:print.S
	nasm -f elf -o lib/print.o print.S
lib/interrupt0.o:interrupt/*.c interrupt/*.S
	nasm -f elf -o lib/interrupt0.o interrupt/interrupt.S 
lib/interrupt1.o:interrupt/*.c interrupt/*.S
	gcc -c -m32 interrupt/interrupt.c -o lib/interrupt1.o  -fno-stack-protector
lib/init.o:init/init.c
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector
lib/timer.o:device/timer.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/debug.o:debug/debug.c
	gcc -m32 -c $^ -I include -o lib/debug.o -fno-stack-protector
lib/memory.o:memop/memory.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/string.o:memop/string.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/bitmap.o:memop/bitmap.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/thread.o:thread/thread.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/threadS.o:thread/thread.S
	nasm -f elf -o $@ $^
lib/list.o:list/list.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/sync.o:thread/sync.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/console.o:console/console.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/keyboard.o:device/keyboard.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/ioqueue.o:device/ioqueue.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/tss.o:userprog/tss.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector
lib/process.o:userprog/process.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector













all:
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector
	nasm -f elf -o lib/print.o print.S
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector
	gcc -m32 -c device/timer.c -I include -o lib/timer.o -fno-stack-protector
	gcc -m32 -c debug/debug.c -I include -o lib/debug.o -fno-stack-protector
	gcc -m32 -c memop/memory.c -I include -o lib/memory.o -fno-stack-protector
	gcc -m32 -c memop/string.c -I include -o lib/string.o -fno-stack-protector
	gcc -m32 -c memop/bitmap.c -I include -o lib/bitmap.o -fno-stack-protector
	gcc -m32 -c thread/thread.c -I include -o lib/thread.o -fno-stack-protector
	nasm -f elf -o lib/threadS.o thread/thread.S
	gcc -m32 -c list/list.c -I include -o lib/list.o -fno-stack-protector
	gcc -m32 -c thread/sync.c -I include -o lib/sync.o -fno-stack-protector
	gcc -m32 -c console/console.c -I include -o lib/console.o -fno-stack-protector
	gcc -m32 -c device/keyboard.c -I include -o lib/keyboard.o -fno-stack-protector
	gcc -m32 -c device/ioqueue.c -I include -o lib/ioqueue.o -fno-stack-protector
	gcc -m32 -c userprog/tss.c -I include -o lib/tss.o -fno-stack-protector
	gcc -m32 -c userprog/process.c -I include -o lib/process.o -fno-stack-protector



install:
	dd if=kernel.bin of=../hd60M.img bs=512 count=200 seek=9 conv=notrunc

	