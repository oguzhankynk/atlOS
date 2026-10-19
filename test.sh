#!/bin/sh
# Build the kernel in self test mode, boot it in QEMU and report the result.
# Exit code: 0 = all tests passed, 1 = failures, 2 = crash/timeout or build error.
#
# Leaves a self test kernel in sysroot and atlOS.iso; run
# "./run.sh build-kernel iso" to get the normal kernel back.

SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd "$SCRIPT_DIR" || exit 2

LOG="$SCRIPT_DIR/selftest.log"
TIMEOUT=60

./libcCMake.sh > /dev/null || { echo "libc build failed"; exit 2; }
./kernelCMake.sh -DATLOS_SELFTEST=ON > /dev/null || { echo "kernel build failed"; exit 2; }
./iso.sh > /dev/null 2>&1 || { echo "iso build failed"; exit 2; }

timeout "$TIMEOUT" qemu-system-x86_64 \
    -cdrom atlOS.iso \
    -m 512M \
    -display none \
    -serial file:"$LOG" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -no-reboot
QEMU_STATUS=$?

grep -a "\[SELFTEST\]" "$LOG"

# isa-debug-exit: QEMU exits with (code << 1) | 1
case $QEMU_STATUS in
    1)
        echo "SELFTEST PASSED"
        exit 0
        ;;
    3)
        echo "SELFTEST FAILED"
        exit 1
        ;;
    124)
        echo "SELFTEST TIMEOUT after ${TIMEOUT}s (panic or hang?), see $LOG"
        exit 2
        ;;
    *)
        echo "SELFTEST CRASHED (qemu exit $QEMU_STATUS), see $LOG"
        exit 2
        ;;
esac
