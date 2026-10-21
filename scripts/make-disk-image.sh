#!/bin/sh
# Create the test disk: a raw image with an MBR and one FAT16 partition,
# holding tools/disk/files/, a generated BIG.TXT and the given programs in
# /BIN (as upper case 8.3 names: hello.elf -> /BIN/HELLO.ELF). No root needed.
# Used by the "disk-image" target; the self tests read it as /disk0.
#
# Usage: scripts/make-disk-image.sh <disk.img> [program.elf...]
# Needs sfdisk (util-linux/fdisk), mkfs.fat (dosfstools) and mtools.

set -e

# sfdisk and mkfs.fat live in sbin, often not on a user's PATH. CMake passes
# the tools it found (SFDISK=... MCOPY=...); otherwise they are looked up here
PATH=$PATH:/usr/sbin:/sbin
SFDISK=${SFDISK:-sfdisk}
MKFS_FAT=${MKFS_FAT:-mkfs.fat}
MCOPY=${MCOPY:-mcopy}
MMD=${MMD:-mmd}

missing=
for tool in "$SFDISK" "$MKFS_FAT" "$MCOPY" "$MMD"; do
    command -v "$tool" > /dev/null 2>&1 || missing="$missing $tool"
done
if [ -n "$missing" ]; then
    echo "$0: not found:$missing" >&2
    echo "On Debian/Ubuntu: sudo apt install mtools dosfstools fdisk" >&2
    exit 1
fi

OUT=$1
shift
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
printf 'label: dos\nstart=%d, type=06\n' "$START" | "$SFDISK" --quiet "$OUT"

# mkfs.fat counts in 1 KB blocks; --offset is in sectors
"$MKFS_FAT" -F 16 -n ATLOS --offset "$START" "$OUT" $(((SECTORS - START) / 2)) > /dev/null

# mtools reaches the partition through "image@@offset"
PART="$OUT@@$((START * 512))"
export MTOOLS_SKIP_CHECK=1

"$MCOPY" -i "$PART" "$FILES/HELLO.TXT" ::/HELLO.TXT
"$MMD" -i "$PART" ::/DOCS
"$MCOPY" -i "$PART" "$FILES/DOCS/README.TXT" ::/DOCS/README.TXT

# 2000 lines of "line 00001\n": a file over many clusters
BIG=$(mktemp)
i=1
while [ $i -le 2000 ]; do
    printf 'line %05d\n' $i
    i=$((i + 1))
done > "$BIG"
"$MCOPY" -i "$PART" "$BIG" ::/BIG.TXT
rm -f "$BIG"

# User programs
if [ $# -gt 0 ]; then
    "$MMD" -i "$PART" ::/BIN
    for program in "$@"; do
        name=$(basename "$program" | tr '[:lower:]' '[:upper:]')
        "$MCOPY" -i "$PART" "$program" "::/BIN/$name"
    done
fi
