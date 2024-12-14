kernel.bin: main.o lib/*.o
	ld -m elf_i386 $^ -Ttext 0xc0001500 -e main -o kernel.bin -nostdlib


main.o:main.c
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector -nostdlib -nostdinc -w
lib/print.o:print.S
	nasm -f elf -o lib/print.o print.S
lib/interrupt0.o:interrupt/*.c interrupt/*.S
	nasm -f elf -o lib/interrupt0.o interrupt/interrupt.S 
lib/interrupt1.o:interrupt/*.c interrupt/*.S
	gcc -c -m32 interrupt/interrupt.c -o lib/interrupt1.o  -fno-stack-protector -nostdlib -nostdinc -w
lib/init.o:init/init.c
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector -nostdlib -nostdinc -w
lib/timer.o:device/timer.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/debug.o:debug/debug.c
	gcc -m32 -c $^ -I include -o lib/debug.o -fno-stack-protector -nostdlib -nostdinc -w
lib/memory.o:memop/memory.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/string.o:memop/string.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/bitmap.o:memop/bitmap.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/thread.o:thread/thread.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/threadS.o:thread/thread.S
	nasm -f elf -o $@ $^
lib/list.o:list/list.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/sync.o:thread/sync.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/console.o:console/console.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/keyboard.o:device/keyboard.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/ioqueue.o:device/ioqueue.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/tss.o:userprog/tss.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/process.o:userprog/process.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/syscall.o:interrupt/syscall.c
	gcc -m32 -c interrupt/syscall.c -I include -o lib/syscall.o -fno-stack-protector -nostdlib -nostdinc -w

lib/stdio.o:standard/stdio.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/stdio-kernel.o:standard/stdio-kernel.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/ide.o: device/ide.c 
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/fs.o: fs/fs.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/dir.o: fs/dir.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/file.o: fs/file.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/inode.o: fs/inode.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

lib/shell.o: shell/shell.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/pipe.o: device/pipe.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/exec.o: userprog/exec.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/editor.o: editor/editor.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/history.o: shell/history.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/log.o: log/log.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/sm4.o: crypto/sm4.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/sha256.o: crypto/sha256.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w
lib/xor.o: crypto/xor.c
	gcc -m32 -c $^ -I include -o $@ -fno-stack-protector -nostdlib -nostdinc -w

all:
	gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector -nostdlib -nostdinc -w
	nasm -f elf -o lib/print.o print.S
	gcc -c -m32 init/init.c -o lib/init.o  -fno-stack-protector -nostdlib -nostdinc -w
	gcc -m32 -c device/timer.c -I include -o lib/timer.o -fno-stack-protector -nostdlib -nostdinc -w
	gcc -m32 -c debug/debug.c -I include -o lib/debug.o -fno-stack-protector -nostdlib -nostdinc -w
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
	gcc -m32 -c log/log.c -I include -o lib/log.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c crypto/sm4.c -I include -o lib/sm4.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c crypto/sha256.c -I include -o lib/sha256.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c crypto/xor.c -I include -o lib/xor.o -fno-stack-protector -nostdlib -nostdinc
install:
	dd if=kernel.bin of=../hd60M.img bs=512 count=300 seek=9 conv=notrunc

	