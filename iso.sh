#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/ParsOS.kernel isodir/boot/ParsOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "ParsOS" {
	multiboot /boot/ParsOS.kernel
}
EOF
grub-mkrescue -o ParsOS.iso isodir
