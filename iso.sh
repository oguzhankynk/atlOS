#!/bin/sh
mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/atlOS.kernel isodir/boot/atlOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "atlOS" {
	multiboot /boot/atlOS.kernel
}
EOF
grub-mkrescue -o isodir/atlOS.iso isodir