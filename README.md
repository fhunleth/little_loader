# Picoboot

```
make
./run_qemu.sh
```

## Debugging with gdb

First, decide whether you want to debug `picoboot` or the Linux kernel. If
debugging `picoboot`, enable symbols in `picoboot.elf` by building with `-g`. If
debugging the Linux kernel, then you'll need `vmlinux`.

Add `-S -s` to the qemu commandline to start it and enable remote debugging to
port `1234`. Qemu will start paused until gdb connects.

Start `gdb` by running:

```
$ $CROSS-gdb picoboot.elf
...
(gdb) target remote :1234
```

Then use gdb like normal.

## Useful references

https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.pdf
https://www.kernel.org/doc/Documentation/arm64/booting.txt

