kernel.bin: main.o lib/*.o
	ld -m elf_i386 $^ -Ttext 0xc0001500 -e main -o kernel.bin -nostdlib


main.o:main.c
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector -nostdlib -nostdinc
lib/print.o:print.S
	nasm -f elf -o lib/print.o print.S
lib/interrupt0.o:interrupt/*.c interrupt/*.S
	nasm -f elf -o lib/interrupt0.o interrupt/interrupt.S 
lib/interrupt1.o:interrupt/*.c interrupt/*.S
	gcc -c -m32 interrupt/interrupt.c -o lib/interrupt1.o  -fno-stack-protector -nostdlib -nostdinc
lib/init.o:init/init.c
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector -nostdlib -nostdinc
lib/timer.o:device/timer.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/debug.o:debug/debug.c
	gcc -m32 -c $^ -I include -o lib/debug.o -fno-stack-protector -nostdlib -nostdinc
lib/memory.o:memop/memory.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/string.o:memop/string.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/bitmap.o:memop/bitmap.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/thread.o:thread/thread.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/threadS.o:thread/thread.S
	nasm -f elf -o $@ $^
lib/list.o:list/list.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/sync.o:thread/sync.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/console.o:console/console.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/keyboard.o:device/keyboard.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/ioqueue.o:device/ioqueue.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/tss.o:userprog/tss.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/process.o:userprog/process.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/syscall.o:interrupt/syscall.c
	gcc -m32 -c interrupt/syscall.c -I include -o lib/syscall.o -fno-stack-protector -nostdlib -nostdinc

lib/stdio.o:standard/stdio.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/stdio-kernel.o:standard/stdio-kernel.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/ide.o: device/ide.c 
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/fs.o: fs/fs.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/dir.o: fs/dir.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/file.o: fs/file.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/inode.o: fs/inode.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

lib/shell.o: shell/shell.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/pipe.o: device/pipe.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/exec.o: userprog/exec.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/editor.o: editor/editor.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc
lib/history.o: shell/history.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc

all:
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector -nostdlib -nostdinc
	nasm -f elf -o lib/print.o print.S
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c device/timer.c -I include -o lib/timer.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c debug/debug.c -I include -o lib/debug.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c memop/memory.c -I include -o lib/memory.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c memop/string.c -I include -o lib/string.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c memop/bitmap.c -I include -o lib/bitmap.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c thread/thread.c -I include -o lib/thread.o -fno-stack-protector -nostdlib -nostdinc
	nasm -f elf -o lib/threadS.o thread/thread.S
	gcc -m32 -c list/list.c -I include -o lib/list.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c thread/sync.c -I include -o lib/sync.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c console/console.c -I include -o lib/console.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c device/keyboard.c -I include -o lib/keyboard.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c device/ioqueue.c -I include -o lib/ioqueue.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c userprog/tss.c -I include -o lib/tss.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c userprog/process.c -I include -o lib/process.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c interrupt/syscall.c -I include -o lib/syscall.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c standard/stdio.c -I include -o lib/stdio.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c standard/stdio-kernel.c -I include -o lib/stdio-kernel.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c device/ide.c  -I include -o lib/ide.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c fs/fs.c -I include -o lib/fs.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c  fs/dir.c -I include -o lib/dir.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c fs/file.c -I include -o lib/file.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c fs/inode.c -I include -o lib/inode.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c shell/shell.c -I include -o lib/shell.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c device/pipe.c -I include -o lib/pipe.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c userprog/exec.c -I include -o lib/exec.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c editor/editor.c -I include -o lib/editor.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c shell/history.c -I include -o lib/history.o -fno-stack-protector -nostdlib -nostdinc

install:
	dd if=kernel.bin of=../hd60M.img bs=512 count=300 seek=9 conv=notrunc

	