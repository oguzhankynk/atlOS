#!/bin/sh
qemu-system-x86_64 \
    -cdrom isodir/atlOS.iso \
    -m 512M \
    -nographic \
    -monitor none \
    -serial stdio \
    -no-shutdown
