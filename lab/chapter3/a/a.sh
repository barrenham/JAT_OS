nasm pmtest1.asm -I. -o pmtest1.bin &&
dd if=pmtest2.bin of=a.img bs=512 count=1 seek=0 conv=notrunc
