#CROSS = aarch64-linux-gnu
CROSS = /Users/fhunleth/.nerves/artifacts/nerves_toolchain_aarch64_nerves_linux_gnu-darwin_arm-13.2.0/bin/aarch64-nerves-linux-gnu

all: disk.img

gdb:
	$(CROSS)-gdb picoboot.elf

gdb-vmlinux:
	$(CROSS)-gdb vmlinux

disk.img: picoboot.elf Image
	dd if=/dev/zero of=disk.img bs=1M count=64
	dd if=Image of=disk.img seek=128 conv=notrunc

picoboot.elf: src/start.o src/main.o src/virtio_blk.o src/pl011_uart.o src/util.o src/uboot_env.o src/crc32.o src/printf.o
	$(CROSS)-ld -T src/linker.ld -z max-page-size=4096 -o $@ $^
	$(CROSS)-strip -s $@

%.o: %.c
	$(CROSS)-gcc -c -O2 -nostdlib -ffreestanding -fno-builtin -o $@ $<

%.o: %.S
	$(CROSS)-as -o $@ $<

virtio_blk.o: virtio.h
main.o: virtio.h

clean:
	rm -f src/*.o *.elf disk.img

