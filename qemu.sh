#!/bin/sh
qemu-system-x86_64 \
    -cdrom atlOS.iso \
    -m 512M \
    -nographic \
    -monitor none \
    -serial stdio \
    -no-shutdown
