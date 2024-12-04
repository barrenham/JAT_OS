nasm -I. loader.s -o loader.bin &&
nasm -I. mbr.s -o mbr.bin &&
dd if=loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc &&
dd if=mbr.bin of=hd60M.img bs=512 count=1 seek=0 conv=notrunc