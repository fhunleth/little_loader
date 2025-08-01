VERSION=0.1.0

CROSS = aarch64-nerves-linux-gnu
# Uncomment when building with Homebrew's AARCH64 cross-compiler
#CROSS = aarch64-elf

# DEBUG = 1

ifeq ($(DEBUG), 1)
CFLAGS += -g -DDEBUG
LDFLAGS += -g
else
# Turn off -O2 for now since it's causes a fault in uart_init
CFLAGS +=
endif

# Floating point instructions are disabled to avoid needing to set
# up support completely when running in EL1. EL2 is fine.
CFLAGS += -nostdlib -ffreestanding -fno-builtin -mgeneral-regs-only -Werror
CFLAGS += -DPROGRAM_VERSION=$(VERSION)
LDFLAGS += -z max-page-size=4096

S_SRC = $(wildcard src/*.S)
C_SRC = $(wildcard src/*.c) $(wildcard src/libfdt/*.c)
OBJS = $(C_SRC:.c=.o) $(S_SRC:.S=.o)

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

picoboot.elf: $(OBJS)
	$(CROSS)-ld -T src/linker.ld $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CROSS)-gcc -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(CROSS)-as -o $@ $<

virtio_blk.o: virtio.h
main.o: virtio.h

clean:
	$(RM) $(OBJS) *.elf disk.img demo/demo.fw

