# Picoboot

## U-Boot environment

The U-Boot environment is hardcoded to be read from LBA 16 with length of 256
blocks (128 KB). There's no support for redundant blocks currently.

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
