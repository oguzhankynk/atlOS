#!/bin/sh

qemu-system-$1 -monitor stdio -cdrom isodir/ParsOS.iso -d int -no-reboot -no-shutdown