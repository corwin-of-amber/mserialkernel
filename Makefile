
kernel: kernel.o kasm.o vga.o
	i386-elf-gcc  -T link.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

kasm.o: kernel.asm
	nasm -f elf32 $^ -o $@

%.o: %.c
	i386-elf-gcc -c $^ -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra
