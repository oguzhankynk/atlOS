#!/bin/sh
# Boot a self test kernel ISO in QEMU and report the result.
# Used by the "selftest" target (scripts/build.sh selftest, ctest --preset selftest).
#
# Usage: scripts/run-selftest.sh <atlOS.iso> [serial.log]
# Exit code: 0 = all tests passed, 1 = failures, 2 = crash, timeout or bad usage.

ISO=$1
LOG=${2:-selftest.log}
TIMEOUT=${SELFTEST_TIMEOUT:-60}

if [ -z "$ISO" ] || [ ! -f "$ISO" ]; then
    echo "usage: $0 <atlOS.iso> [serial.log]" >&2
    exit 2
fi

timeout "$TIMEOUT" qemu-system-x86_64 \
    -cdrom "$ISO" \
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
