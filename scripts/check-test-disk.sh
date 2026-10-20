#!/bin/sh
# Check the test disk after the self tests wrote to it, with tools that are
# not atlOS: fsck.fat must find the FAT16 volume consistent, and mtools must
# read the files the kernel wrote (see testAtaDisk in kernel/tests/selftest.cpp).
#
# Usage: scripts/check-test-disk.sh <disk.img>
# Exit code: 0 = consistent and as expected, 1 = not.

DISK=$1
PATH=$PATH:/usr/sbin:/sbin
export MTOOLS_SKIP_CHECK=1

if [ -z "$DISK" ] || [ ! -f "$DISK" ]; then
    echo "usage: $0 <disk.img>" >&2
    exit 1
fi

# The partition of scripts/make-disk-image.sh: from sector 2048 to the end
PART="$DISK@@1048576"
VOLUME=$(mktemp)
trap 'rm -f "$VOLUME" "$VOLUME.expected"' EXIT
FAILED=0

check() {
    if [ "$1" -eq 0 ]; then
        echo "  ok:      $2"
    else
        echo "  wrong:   $2"
        FAILED=1
    fi
}

# fsck.fat has no offset option: check a copy of the partition, read only (-n)
dd if="$DISK" of="$VOLUME" bs=512 skip=2048 status=none
fsck.fat -n "$VOLUME" > "$VOLUME.expected" 2>&1
STATUS=$?
if [ $STATUS -ne 0 ]; then
    cat "$VOLUME.expected"
fi
check $STATUS "fsck.fat finds the volume consistent"

[ "$(mtype -i "$PART" ::/KERNEL/NOTE.TXT)" = "written by atlOS" ]
check $? "mtype KERNEL/NOTE.TXT"

i=1
while [ $i -le 1000 ]; do
    printf 'data %05d\n' $i
    i=$((i + 1))
done > "$VOLUME.expected"
mtype -i "$PART" ::/KERNEL/DATA.TXT | cmp -s - "$VOLUME.expected"
check $? "mtype KERNEL/DATA.TXT (11000 bytes)"

! mdir -i "$PART" ::/KERNEL/TMP > /dev/null 2>&1
check $? "KERNEL/TMP was removed"

[ "$(mtype -i "$PART" ::/HELLO.TXT)" = "Hello from the atlOS disk!" ]
check $? "HELLO.TXT is untouched"

exit $FAILED
