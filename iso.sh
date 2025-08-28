#!/bin/sh

if command -v grub2-mkrescue >/dev/null 2>&1; then
    GRUB_MKRESCUE=grub2-mkrescue
elif command -v grub-mkrescue >/dev/null 2>&1; then
    GRUB_MKRESCUE=grub-mkrescue
else
    echo "Cannot find grub-mkrescue or grub2-mkrescue"
    exit 1
fi

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/atlOS.kernel isodir/boot/atlOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "atlOS" {
	multiboot2 /boot/atlOS.kernel
}
EOF
"$GRUB_MKRESCUE" -o isodir/atlOS.iso isodir