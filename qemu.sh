#!/bin/sh



qemu-system-$1 -monitor stdio -cdrom isodir/atlOS.iso -d int -no-reboot -no-shutdown #-serial  telnet:localhost:4321,server,nowait