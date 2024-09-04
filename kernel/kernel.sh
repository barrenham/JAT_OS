cd device &&
sh timer.sh &&
cd .. &&
cd interrupt &&
sh  interrupt.sh &&
cd .. &&
cd init &&
sh init.sh &&
cd .. &&
nasm -f elf -o lib/print.o print.S &&
gcc -I include -m32 -c -o main.o main.c  -fno-stack-protector && 
ld -m elf_i386 main.o lib/print.o lib/interrupt0.o lib/interrupt1.o lib/init.o lib/timer.o -Ttext 0xc0001500 -e main -o kernel.bin &&
dd if=kernel.bin of=../hd60M.img bs=512 count=200 seek=9 conv=notrunc