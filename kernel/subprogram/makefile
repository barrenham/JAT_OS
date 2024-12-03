test: test.c stdio.c string.c
	gcc -m32 -c test.c  -o test.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c stdio.c  -o stdio.o -fno-stack-protector -nostdlib -nostdinc
	gcc -m32 -c string.c  -o string.o -fno-stack-protector -nostdlib -nostdinc
install: test
	ld -m elf_i386 -Ttext 0x8050000 -e main -o prog.bin test.o stdio.o string.o -nostdlib
	dd if=prog.bin of=../../hd60M.img bs=512 count=200 seek=400 conv=notrunc