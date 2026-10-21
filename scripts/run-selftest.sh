#!/bin/sh
# Boot a self test kernel ISO in QEMU and report the result.
# Used by the "selftest" target (scripts/build.sh selftest, ctest --preset selftest).
#
# Usage: scripts/run-selftest.sh <atlOS.iso> [serial.log] [disk.img]
#   With a disk image (scripts/make-disk-image.sh) a copy is attached as the
#   primary master, the disk tests must run (not be skipped), and after a
#   pass scripts/check-test-disk.sh checks what the kernel wrote to it.
# Exit code: 0 = all tests passed, 1 = failures, 2 = crash, timeout or bad usage.

ISO=$1
LOG=${2:-selftest.log}
DISK=$3
TIMEOUT=${SELFTEST_TIMEOUT:-60}

if [ -z "$ISO" ] || [ ! -f "$ISO" ]; then
    echo "usage: $0 <atlOS.iso> [serial.log] [disk.img]" >&2
    exit 2
fi

# A copy: the tests write to the disk
DISK_ARGS=
if [ -n "$DISK" ]; then
    cp "$DISK" "$LOG.disk.img"
    DISK_ARGS="-drive file=$LOG.disk.img,format=raw,if=ide,index=0 -boot d"
fi

timeout "$TIMEOUT" qemu-system-x86_64 \
    -cpu "${QEMU_CPU:-$(cat "$(dirname "$0")/qemu-cpu")}" \
    -cdrom "$ISO" $DISK_ARGS \
    -m 512M \
    -display none \
    -serial file:"$LOG" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -no-reboot
QEMU_STATUS=$?

grep -a "\[SELFTEST\]" "$LOG"

if [ -n "$DISK" ]; then
    if grep -aq "skipped: no ATA disk" "$LOG"; then
        rm -f "$LOG.disk.img"
        echo "SELFTEST FAILED: a disk was attached but the kernel did not find it"
        exit 1
    fi

    # Only a passed run has written everything the check expects
    if [ $QEMU_STATUS -eq 1 ]; then
        echo "Checking the disk the kernel wrote:"
        if ! "$(dirname "$0")/check-test-disk.sh" "$LOG.disk.img"; then
            echo "SELFTEST FAILED: the disk is not as the kernel claims, kept as $LOG.disk.img"
            exit 1
        fi
    fi
    rm -f "$LOG.disk.img"
fi

# isa-debug-exit: QEMU exits with (code << 1) | 1, code = Core::ExitStatus
# written to ATLOS_DEBUG_EXIT_PORT (0 success, 1 failure, 2 panic)
case $QEMU_STATUS in
    1)
        echo "SELFTEST PASSED"
        exit 0
        ;;
    3)
        echo "SELFTEST FAILED"
        exit 1
        ;;
    5)
        echo "SELFTEST PANIC, see $LOG"
        exit 2
        ;;
    124)
        echo "SELFTEST TIMEOUT after ${TIMEOUT}s (hang?), see $LOG"
        exit 2
        ;;
    *)
        echo "SELFTEST CRASHED (qemu exit $QEMU_STATUS), see $LOG"
        exit 2
        ;;
esac
