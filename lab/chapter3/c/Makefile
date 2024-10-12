##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=pmtest3.asm
BIN:=$(subst .asm,.com,$(SRC))

.PHONY : everything

everything : $(BIN)
	sudo mount -o loop pm.img mnt
	sudo cp $(BIN) mnt -v
	sudo umount mnt

$(BIN) : $(SRC)
	nasm $< -o $@

