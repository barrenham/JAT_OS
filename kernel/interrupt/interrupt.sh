nasm -f elf -o ../lib/interrupt0.o interrupt.S &&
gcc -c -m32 interrupt.c -o ../lib/interrupt1.o  -fno-stack-protector