#!/bin/sh

qemu-system-aarch64 \
    -M virt,virtualization=on -cpu max -nographic -smp 1 \
    -kernel picoboot.elf \
    -S -s \
    -drive if=none,file=disk.img,format=raw,id=vdisk \
    -global virtio-mmio.force-legacy=false \
    -device virtio-blk-device,drive=vdisk,bus=virtio-mmio-bus.0

