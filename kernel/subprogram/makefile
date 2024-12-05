test: test.c stdio.c string.c syscall.c
	gcc -m32 -c test.c  -o lib/test.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c stdio.c  -o lib/stdio.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c string.c  -o lib/string.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c syscall.c -o lib/syscall.o -fno-stack-protector -nostdlib -nostdinc
install: test
	ld -m elf_i386 -Ttext 0x8050000 -e main -o prog.bin lib/test.o lib/stdio.o lib/string.o lib/syscall.o -nostdlib
	dd if=prog.bin of=../../hd60M.img bs=512 count=200 seek=400 conv=notrunc