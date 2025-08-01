# Picoboot

Picoboot is a minimal bootloader to support loading a Linux kernel from a
disk image that's been prepared with A/B firmware images. It only is
usable with Qemu. The whole implementation is so simple that it is hoped
to be possible to follow the code without a ton of effort.

## Demo

There's a simple Linux image included for trying out the bootloader. Here's
an example run:

```sh
$ ./run_qemu_el1.sh
picoboot 0.1.0
Running in EL1
Booting from slot a with kernel LBA 8192
Starting Linux...
Booting Linux on physical CPU 0x0000000000 [0x410fd034]
Linux version 6.12.27 (fhunleth@sprint) (aarch64-linux-gcc.br_real (Buildroot 2021.11-12449-g1bef613319) 14.2.0, GNU ld (GNU Binutils) 2.42) #10 SMP Sat Jul 26 10:40:14 EDT 2025

...


Welcome to Buildroot
buildroot login:
```

The login is `root`. To exit, run `poweroff`.

To try this out for yourself, see the section below about building from source.

## Disk image layout

Picoboot expects a disk image with the following layout:

| LBA (512-byte block offset) | Description                      |
| --------------------------- | -------------------------------- |
| 0                           | MBR or GPT                       |
| 16                          | U-Boot environment block (128KB) |
| n                           | Linux kernel for slot A          |
| m                           | Linux kernel for slot B          |
| ...                         | RootFS and data partitions       |

The locations for where to find the Linux kernels (the `Image` files) are
read from `a.kernel_lba` and `b.kernel_lba` in the U-Boot environment block.
The variable `nerves_fw_active` should be set to either `a` or `b` to select
which slot is loaded.

## U-Boot environment

The A/B upgrade mechanism uses a mix of the U-Boot bootcount mechanism with
Nerves A/B slot tracking variables. While this looks inconsistent, it more
closely matches Nerves systems that use U-Boot to manage A/B software updates.

The following environment variables are meaningful:

* `nerves_fw_active` - which firmware slot to load. Set to `a` or `b`
* `upgrade_available` - set to `"1"` when new firmware is available
* `bootcount` - when `upgrade_available` is `"1"`, this gets incremented on each
   boot. When > 1 (hardcoded now), the other firmware slot will be booted from
   then on. I.e., the firmware will be reverted.
* `<slot>.kernel_lba` - offset of the Linux kernel on disk
* `<slot>.nerves_fw_validated` - `"0"` if not validated, `"1"` if validated

## Building from source

First install `fwup` and `qemu-system`. On Homebrew, this is:

```sh
brew install fwup qemu
```

Then install an ARM64 cross-compiler. Any one should do. If you use `asdf`,
then you can get one of the Nerves toolchain ones:

```sh
asdf plugin add nerves-toolchain

# Next line is optional, but you'll quickly see.
export GITHUB_API_TOKEN=$(gh auth token)

asdf install
```

Another perfectly fine ARM64 cross-compiler can be installed with Homebrew:

```sh
brew install aarch64-elf-gcc
```

If you want to use the Homebrew cross-compiler, update the value of `CROSS` in
the `Makefile`.

Here's how to run with the provided test image:

```sh
make
./run_qemu_el1.sh
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
