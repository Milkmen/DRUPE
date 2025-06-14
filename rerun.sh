cd ./scripts
bash build_example.sh
cd ..

make clean
make all
qemu-system-i386 -cdrom kernel.iso
