#!/bin/sh

set -e

# Adjust commandline parameters to match desired config
qemu-system-aarch64 \
    -M virt,virtualization=on -cpu cortex-a53 -nographic -smp 1 \
    -kernel picoboot.elf \
    -machine dumpdtb=virt.dtb \
    -global virtio-mmio.force-legacy=false \
    -drive if=none,file=disk.img,format=raw,id=vdisk \
    -device virtio-blk-device,drive=vdisk,bus=virtio-mmio-bus.0

dtc -I dtb -O dts virt.dtb > virt.dtc

