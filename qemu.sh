#!/bin/sh

# Launch QEMU with the following options:
# Enable QEMU monitor on standard input/output
# Boot from the first CD-ROM drive
# Use atlOS.iso as the CD-ROM image
# Attach a raw disk image as a hard drive
# Emulate a 64-bit QEMU CPU
# Allocate 512 MB of RAM to the VM
# Enable logging of interrupts
# Do not reboot after shutdown
# Do not exit QEMU on guest shutdown
qemu-system-x86_64 \
    -cdrom isodir/atlOS.iso \
    -serial stdio \
    -m 1024M \
    -no-shutdown \
    -serial  telnet:localhost:4321,server,nowait

#-serial  telnet:localhost:4321,server,nowait  # (Optional) Enable serial port over telnet for debugging
