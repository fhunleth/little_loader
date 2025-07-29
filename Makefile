#CROSS = aarch64-linux-gnu
CROSS = aarch64-elf

# DEBUG = 1

ifeq ($(DEBUG), 1)
CFLAGS += -g -DDEBUG
LDFLAGS += -g
else
CFLAGS += -O2
endif
CFLAGS += -nostdlib -ffreestanding -fno-builtin
CFLAGS += -DPROGRAM_VERSION=0.1.0
LDFLAGS += -z max-page-size=4096

all: picoboot.elf disk.img

gdb:
	$(CROSS)-gdb picoboot.elf

gdb-vmlinux:
	$(CROSS)-gdb vmlinux

disk.img: demo.fw
	fwup demo.fw -d disk.img

demo.fw: demo/Image demo/fwup.conf
	fwup -c -f demo/fwup.conf -o demo.fw

upgrade: demo.fw
	fwup demo.fw -d disk.img -t upgrade

picoboot.elf: src/start.o src/main.o src/virtio_blk.o src/pl011_uart.o src/util.o src/uboot_env.o src/crc32.o src/printf.o
	$(CROSS)-ld -T src/linker.ld $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CROSS)-gcc -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(CROSS)-as -o $@ $<

virtio_blk.o: virtio.h
main.o: virtio.h

clean:
	rm -f src/*.o *.elf disk.img demo/demo.fw

