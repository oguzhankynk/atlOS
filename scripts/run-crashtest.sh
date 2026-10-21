#!/bin/sh
# Boot a crash test kernel ISO (preset "crashtest") in QEMU: the kernel reads an
# unmapped address on purpose, and its fault report must say so.
# Used by the "crashtest" target (scripts/build.sh crashtest, ctest --preset crashtest).
#
# Usage: scripts/run-crashtest.sh <atlOS.iso> [serial.log] [atlOS.kernel]
#   With the kernel image, the backtrace is also symbolized (scripts/symbolize.sh).
# Exit code: 0 = the report is as expected, 1 = it is not, 2 = timeout or bad usage.

ISO=$1
LOG=${2:-crashtest.log}
KERNEL=$3
TIMEOUT=${CRASHTEST_TIMEOUT:-60}
ROOT=$(cd "$(dirname "$0")/.." && pwd)

if [ -z "$ISO" ] || [ ! -f "$ISO" ]; then
    echo "usage: $0 <atlOS.iso> [serial.log] [atlOS.kernel]" >&2
    exit 2
fi

timeout "$TIMEOUT" qemu-system-x86_64 \
    -cpu "${QEMU_CPU:-$(cat "$(dirname "$0")/qemu-cpu")}" \
    -cdrom "$ISO" \
    -m 512M \
    -display none \
    -serial file:"$LOG" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -no-reboot
QEMU_STATUS=$?

FAILED=0

expect() {
    if grep -aqF -- "$1" "$2"; then
        echo "  ok:      $1"
    else
        echo "  missing: $1"
        FAILED=1
    fi
}

# isa-debug-exit: (code << 1) | 1, Core::ExitStatus::Panic = 2
case $QEMU_STATUS in
    5) echo "  ok:      halted with the panic status" ;;
    124)
        echo "CRASHTEST TIMEOUT after ${TIMEOUT}s, see $LOG"
        exit 2
        ;;
    *)
        echo "  wrong:   exit status $QEMU_STATUS, expected 5 (panic)"
        FAILED=1
        ;;
esac

expect "EXCEPTION 14: Page fault" "$LOG"
expect "CR2=0x00000000dead0000 (kernel read, page not present)" "$LOG"
expect "KERNEL PANIC" "$LOG"
expect "#1 0x" "$LOG"

if [ -n "$KERNEL" ]; then
    SYMBOLS=$(dirname "$LOG")/crashtest.symbols
    "$ROOT/scripts/symbolize.sh" "$KERNEL" "$LOG" > "$SYMBOLS"
    cat "$SYMBOLS"
    expect "CrashTest::readUnmapped" "$SYMBOLS"
    expect "CrashTest::run" "$SYMBOLS"
fi

if [ $FAILED -eq 0 ]; then
    echo "CRASHTEST PASSED"
else
    echo "CRASHTEST FAILED, see $LOG"
fi
exit $FAILED
