cd kernel &&
make all &&
make &&
make install &&
cd .. &&
./loader.sh &&
bochs -f ./bochsrc.disk