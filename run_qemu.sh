#!/bin/sh

# Cheatsheet
#
# 1. Run on MacOS with acceleration
#    -M virt,accel=hvf -cpu host
#
# 2. Get the device tree
# .  -machine dumpdtb=virt.dtb
#
# 3. Turn on qemu tracing
#    -d in_asm,int,cpu_reset -D qemu.log
#
# 4. Start with EL2 rather than EL1
#    -M virt,virtualization=on
#

qemu-system-aarch64 \
    -M virt -cpu cortex-a53 -nographic -smp 1 \
    -kernel little_loader.elf \
    -global virtio-mmio.force-legacy=false \
    -drive if=none,file=disk.img,format=raw,id=vdisk \
    -device virtio-blk-device,drive=vdisk,bus=virtio-mmio-bus.0


