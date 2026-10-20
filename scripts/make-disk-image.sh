#!/bin/sh
# Create the test disk: a raw image with an MBR and one FAT16 partition,
# holding tools/disk/files/ and a generated BIG.TXT. No root needed.
# Used by the "disk-image" target; the self tests read it as /disk0.
#
# Usage: scripts/make-disk-image.sh <disk.img>
# Needs sfdisk (util-linux/fdisk), mkfs.fat (dosfstools) and mtools.

set -e

# sfdisk and mkfs.fat live in sbin, often not on a user's PATH
PATH=$PATH:/usr/sbin:/sbin

OUT=$1
SIZE_MB=${DISK_SIZE_MB:-32}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
FILES=$ROOT/tools/disk/files

if [ -z "$OUT" ]; then
    echo "usage: $0 <disk.img>" >&2
    exit 2
fi

# The partition starts at 1 MB (sector 2048), as partitioning tools align it
START=2048
SECTORS=$((SIZE_MB * 2048))

rm -f "$OUT"
truncate -s "${SIZE_MB}M" "$OUT"

# One primary partition of type 0x06 (FAT16) over the rest of the disk
printf 'label: dos\nstart=%d, type=06\n' "$START" | sfdisk --quiet "$OUT"

# mkfs.fat counts in 1 KB blocks; --offset is in sectors
mkfs.fat -F 16 -n ATLOS --offset "$START" "$OUT" $(((SECTORS - START) / 2)) > /dev/null

# mtools reaches the partition through "image@@offset"
PART="$OUT@@$((START * 512))"
export MTOOLS_SKIP_CHECK=1

mcopy -i "$PART" "$FILES/HELLO.TXT" ::/HELLO.TXT
mmd -i "$PART" ::/DOCS
mcopy -i "$PART" "$FILES/DOCS/README.TXT" ::/DOCS/README.TXT

# 2000 lines of "line 00001\n": a file over many clusters
BIG=$(mktemp)
i=1
while [ $i -le 2000 ]; do
    printf 'line %05d\n' $i
    i=$((i + 1))
done > "$BIG"
mcopy -i "$PART" "$BIG" ::/BIG.TXT
rm -f "$BIG"
