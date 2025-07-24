#!/bin/sh

dd if=/dev/zero of=disk.img bs=1M count=64

# Insert MBR with boot signature
printf '\x00%.510s\x55\xAA' "" | dd of=disk.img count=1 conv=notrunc

# Insert bootloader binary after MBR
dd if=bootloader.bin of=disk.img seek=1 conv=notrunc

# Insert raw Image kernel at offset 0x10000 (sector 128)
dd if=Image of=disk.img seek=128 conv=notrunc

