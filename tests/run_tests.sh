#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2025 Frank Hunleth
#
# SPDX-License-Identifier: BSD-3-Clause

# "readlink -f" implementation for BSD
# This code was extracted from the Elixir shell scripts
readlink_f () {
    cd "$(dirname "$1")" > /dev/null
    filename="$(basename "$1")"
    if [ -h "$filename" ]; then
        readlink_f "$(readlink "$filename")"
    else
        echo "`pwd -P`/$filename"
    fi
}

TESTS_DIR=$(dirname $(readlink_f $0))

WORK=$TESTS_DIR/work
HOSTSHARE=$WORK/hostshare
DISK_IMAGE=$WORK/disk.img
AUTORUN_SH=$HOSTSHARE/autorun.sh

LITTLE_LOADER=$TESTS_DIR/../little_loader.elf
DEMO_FW=$TESTS_DIR/../demo.fw
FWUP=$(which fwup)

# Collect the tests from the commandline
TESTS=$*
if [ -z "$TESTS" ]; then
    TESTS=$(ls $TESTS_DIR/[0-9][0-9][0-9]_*)
fi

if [ ! -f "$LITTLE_LOADER" ]; then echo "Build $LITTLE_LOADER first"; exit 1; fi
if [ ! -f "$DEMO_FW" ]; then echo "Build $DEMO_FW first"; exit 1; fi
if [ -z "$FWUP" ]; then echo "Install fwup first"; exit 1; fi

# Host-specific options
case $(uname -s) in
    Darwin)
        TIMEOUT=gtimeout
        ;;
    *)
        TIMEOUT=timeout
        ;;
esac

timeout() {
  TIMEOUT=$1
  shift

  # Start the command in the background
  "$@" &
  CMD_PID=$!

  # Start a background timer
  (
    sleep "$TIMEOUT"
    kill -s TERM "$CMD_PID" 2>/dev/null
  ) &
  TIMER_PID=$!

  # Wait for the command to finish
  wait "$CMD_PID"
  CMD_STATUS=$?

  # Kill the timer
  kill "$TIMER_PID" 2>/dev/null

  return $CMD_STATUS
}

run() {
    TEST=$1
    QEMU_MACHINE=virt
    QEMU_CPU=cortex-a53

    echo Running $TEST...

    rm -fr "$WORK"
    mkdir -p "$HOSTSHARE"
    touch "$AUTORUN_SH"
    chmod +x "$AUTORUN_SH"

    # Run the test script to setup files for the test
    source "$TESTS_DIR/$TEST"

    QEMU_ARGS="-M $QEMU_MACHINE -cpu $QEMU_CPU -nographic"
    QEMU_ARGS+=" -smp 1"
    QEMU_ARGS+=" -kernel $LITTLE_LOADER"
    QEMU_ARGS+=" -global virtio-mmio.force-legacy=false"
    QEMU_ARGS+=" -drive if=none,file=$DISK_IMAGE,format=raw,id=vdisk"
    QEMU_ARGS+=" -device virtio-blk-device,drive=vdisk,bus=virtio-mmio-bus.0"
    QEMU_ARGS+=" -virtfs local,path=$HOSTSHARE,mount_tag=hostshare,security_model=none,id=hostshare"

    if [ ! -e "$DISK_IMAGE" ]; then
        echo "Test never created $DISK_IMAGE"
        exit 1
    fi

    # Run qemu
    echo qemu-system-aarch64 "${CMDLINE_ARGS[*]}"

#$TIMEOUT 10 qemu-system-aarch64 -M virt -cpu cortex-a53 -nographic -smp 1 -kernel "$LITTLE_LOADER" -global virtio-mmio.force-legacy=false -drive if=none,file="$DISK_IMAGE",format=raw,id=vdisk -device virtio-blk-device,drive=vdisk,bus=virtio-mmio-bus.0 -virtfs local,path="$HOSTSHARE",mount_tag=hostshare,security_model=none,id=hostshare

    if ! timeout 10 qemu-system-aarch64 $QEMU_ARGS; then
        echo "QEMU failed or timed out when running $TEST"
        exit 1
    fi

    # check results
    if [ ! -e "$HOSTSHARE/success" ]; then
        echo "Didn't find $HOSTSHARE/success file. Test failed."
        exit 1
    fi
}

# Test command line arguments
for TEST_CONFIG in $TESTS; do
    TEST=$(/usr/bin/basename "$TEST_CONFIG" .expected)
    run "$TEST"
done

rm -fr "$WORK"
echo Pass!
exit 0
