#!/bin/sh



qemu-system-$1 -monitor stdio -boot d -cdrom isodir/atlOS.iso -drive file=VirtualDisk/disk.img,media=disk,format=raw -d int -no-reboot -no-shutdown #-serial  telnet:localhost:4321,server,nowait