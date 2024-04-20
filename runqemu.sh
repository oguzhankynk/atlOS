#!/bin/sh
mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/ParsOS.kernel isodir/boot/ParsOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "ParsOS" {
	multiboot /boot/ParsOS.kernel
}
EOF
grub-mkrescue -o isodir/ParsOS.iso isodir

TARGET=i686-elf

if echo "$TARGET" | grep -Eq 'i[[:digit:]]86-'; then
  ARCH=i386
else
  ARCH=$(echo "$TARGET" | grep -Eo '^[[:alnum:]_]*')
fi

# QEMU sistemi çalıştırma
qemu-system-$ARCH -cdrom isodir/ParsOS.iso